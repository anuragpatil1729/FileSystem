#include "filesystem.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <iterator>
#include <type_traits>
#include <vector>

namespace {
constexpr std::size_t B = DiskManager::BLOCK_SIZE;
constexpr std::size_t DIRECTORY_NAME_BYTES = 252;

struct DiskDirectoryEntry {
    std::uint32_t inodeNumber;
    char name[DIRECTORY_NAME_BYTES];
};

struct DiskDirectoryBlock {
    std::uint32_t count;
    DiskDirectoryEntry entries[(B - sizeof(std::uint32_t)) / sizeof(DiskDirectoryEntry)];
};

static_assert(sizeof(DiskDirectoryBlock) <= B, "Directory metadata must fit in one block");
static_assert(std::is_trivially_copyable<Superblock>::value,
              "Superblock must be trivially copyable for this prototype format");
static_assert(std::is_trivially_copyable<Inode>::value,
              "Inode must be trivially copyable for this prototype format");

bool writeBytesToBlocks(DiskManager& disk, std::size_t firstBlock,
                        std::size_t blockCount, const void* source,
                        std::size_t byteCount) {
    const auto capacity = blockCount * B;
    if (byteCount > capacity) return false;

    const auto* bytes = static_cast<const unsigned char*>(source);
    std::vector<unsigned char> buffer(capacity, 0);
    if (byteCount != 0) std::memcpy(buffer.data(), bytes, byteCount);

    for (std::size_t i = 0; i < blockCount; ++i) {
        if (!disk.writeBlock(firstBlock + i, buffer.data() + i * B)) return false;
    }
    return true;
}

bool readBytesFromBlocks(DiskManager& disk, std::size_t firstBlock,
                         std::size_t blockCount, void* destination,
                         std::size_t byteCount) {
    const auto capacity = blockCount * B;
    if (byteCount > capacity) return false;

    std::vector<unsigned char> buffer(capacity, 0);
    for (std::size_t i = 0; i < blockCount; ++i) {
        if (!disk.readBlock(firstBlock + i, buffer.data() + i * B)) return false;
    }
    if (byteCount != 0) std::memcpy(destination, buffer.data(), byteCount);
    return true;
}
}

FileSystem::FileSystem(const std::string& diskPath)
    : disk_(diskPath), mounted_(false) {}

bool FileSystem::format(std::size_t blockCount, std::uint32_t inodeCount) {
    if (blockCount == 0 || inodeCount == 0 ||
        blockCount > std::numeric_limits<std::uint32_t>::max()) return false;

    DiskManager newDisk(disk_.getDiskPath(), blockCount);
    if (!newDisk.initialize()) return false;

    superblock_.initialize(static_cast<std::uint32_t>(blockCount), inodeCount);
    if (!superblock_.isValid() || superblock_.dataBlockStart >= blockCount) return false;

    inodeBitmap_.initialize(inodeCount);
    blockBitmap_.initialize(blockCount);
    inodes_.clear();
    inodes_.resize(inodeCount);
    rootDirectory_ = Directory{};

    // Reserve the superblock, both bitmaps, and inode-table blocks.
    blockBitmap_.set(0);
    blockBitmap_.set(superblock_.inodeBitmapStart);
    blockBitmap_.set(superblock_.blockBitmapStart);
    for (std::size_t i = 0; i < superblock_.inodeTableBlocks; ++i) {
        blockBitmap_.set(superblock_.inodeTableStart + i);
    }

    const int root = allocateInode(InodeType::DIRECTORY);
    if (root != 0) return false;

    // The root directory itself occupies one data block so its entries persist.
    const int directoryBlock = allocateBlock();
    if (directoryBlock < 0) return false;
    inodes_[0].directBlocks[0] = static_cast<std::uint32_t>(directoryBlock);

    mounted_ = true;
    if (!persistMetadata()) {
        mounted_ = false;
        return false;
    }
    return true;
}

bool FileSystem::mount() {
    if (mounted_) return true;
    if (!loadMetadata()) return false;
    mounted_ = true;
    return true;
}

bool FileSystem::unmount() {
    if (!mounted_) return true;
    if (!persistMetadata()) return false;
    mounted_ = false;
    return true;
}

int FileSystem::allocateInode(InodeType type) {
    const std::size_t index = inodeBitmap_.findFree();
    if (index >= inodeBitmap_.size()) return -1;
    if (!inodeBitmap_.set(index)) return -1;
    inodes_[index].initialize(static_cast<std::uint32_t>(index), type);
    return static_cast<int>(index);
}

int FileSystem::allocateBlock() {
    for (std::size_t i = superblock_.dataBlockStart; i < blockBitmap_.size(); ++i) {
        if (blockBitmap_.isFree(i)) {
            if (!blockBitmap_.set(i)) return -1;
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool FileSystem::freeBlock(std::size_t blockNumber) {
    if (blockNumber < superblock_.dataBlockStart || blockNumber >= blockBitmap_.size())
        return false;
    return blockBitmap_.clear(blockNumber);
}

std::string FileSystem::normalizePath(const std::string& path) const {
    if (path.empty()) return "/";
    return path.front() == '/' ? path : "/" + path;
}

int FileSystem::findInode(const std::string& path) const {
    const std::string normalized = normalizePath(path);
    if (normalized == "/") return inodes_.empty() ? -1 : 0;
    if (normalized.size() <= 1 || normalized.find('/', 1) != std::string::npos) return -1;
    return rootDirectory_.findEntry(normalized.substr(1));
}

bool FileSystem::createFile(const std::string& path) {
    if (!mounted_) return false;
    const std::string normalized = normalizePath(path);
    if (normalized == "/" || normalized.find('/', 1) != std::string::npos) return false;
    const std::string name = normalized.substr(1);
    if (name.empty() || name.size() >= DIRECTORY_NAME_BYTES || rootDirectory_.exists(name)) return false;

    const int inode = allocateInode(InodeType::FILE);
    if (inode < 0) return false;
    if (!rootDirectory_.addEntry(name, static_cast<std::uint32_t>(inode))) {
        inodeBitmap_.clear(static_cast<std::size_t>(inode));
        inodes_[static_cast<std::size_t>(inode)] = Inode{};
        return false;
    }
    if (!persistMetadata()) {
        rootDirectory_.removeEntry(name);
        inodeBitmap_.clear(static_cast<std::size_t>(inode));
        inodes_[static_cast<std::size_t>(inode)] = Inode{};
        return false;
    }
    return true;
}

bool FileSystem::createDirectory(const std::string& path) {
    if (!mounted_) return false;
    const std::string normalized = normalizePath(path);
    if (normalized == "/" || normalized.find('/', 1) != std::string::npos) return false;
    const std::string name = normalized.substr(1);
    if (name.empty() || name.size() >= DIRECTORY_NAME_BYTES || rootDirectory_.exists(name)) return false;

    const int inode = allocateInode(InodeType::DIRECTORY);
    if (inode < 0) return false;
    if (!rootDirectory_.addEntry(name, static_cast<std::uint32_t>(inode))) {
        inodeBitmap_.clear(static_cast<std::size_t>(inode));
        inodes_[static_cast<std::size_t>(inode)] = Inode{};
        return false;
    }
    if (!persistMetadata()) {
        rootDirectory_.removeEntry(name);
        inodeBitmap_.clear(static_cast<std::size_t>(inode));
        inodes_[static_cast<std::size_t>(inode)] = Inode{};
        return false;
    }
    return true;
}

bool FileSystem::writeFile(const std::string& path, const std::string& data) {
    if (!mounted_ || data.size() > B) return false;
    const int index = findInode(path);
    if (index < 0) return false;
    Inode& inode = inodes_[static_cast<std::size_t>(index)];
    if (!inode.isFile()) return false;

    bool allocatedNow = false;
    if (inode.directBlocks[0] == 0) {
        const int block = allocateBlock();
        if (block < 0) return false;
        inode.directBlocks[0] = static_cast<std::uint32_t>(block);
        allocatedNow = true;
    }

    std::vector<char> buffer(B, 0);
    if (!data.empty()) std::memcpy(buffer.data(), data.data(), data.size());
    if (!disk_.writeBlock(inode.directBlocks[0], buffer.data())) {
        if (allocatedNow) {
            freeBlock(inode.directBlocks[0]);
            inode.directBlocks[0] = 0;
        }
        return false;
    }
    inode.size = static_cast<std::uint32_t>(data.size());
    if (!persistMetadata()) return false;
    return true;
}

std::string FileSystem::readFile(const std::string& path) {
    if (!mounted_) return {};
    const int index = findInode(path);
    if (index < 0) return {};
    const Inode& inode = inodes_[static_cast<std::size_t>(index)];
    if (!inode.isFile() || inode.size == 0 || inode.directBlocks[0] == 0 || inode.size > B)
        return {};

    std::vector<char> buffer(B, 0);
    if (!disk_.readBlock(inode.directBlocks[0], buffer.data())) return {};
    return std::string(buffer.data(), inode.size);
}

bool FileSystem::deleteFile(const std::string& path) {
    if (!mounted_) return false;
    const std::string normalized = normalizePath(path);
    if (normalized == "/" || normalized.find('/', 1) != std::string::npos) return false;
    const std::string name = normalized.substr(1);
    const int index = findInode(path);
    if (index <= 0) return false;
    Inode& inode = inodes_[static_cast<std::size_t>(index)];
    if (!inode.isFile()) return false;

    if (inode.directBlocks[0] != 0) freeBlock(inode.directBlocks[0]);
    inode = Inode{};
    inodeBitmap_.clear(static_cast<std::size_t>(index));
    if (!rootDirectory_.removeEntry(name)) return false;
    return persistMetadata();
}

bool FileSystem::deleteDirectory(const std::string& path) {
    if (!mounted_) return false;
    const std::string normalized = normalizePath(path);
    if (normalized == "/" || normalized.find('/', 1) != std::string::npos) return false;
    const std::string name = normalized.substr(1);
    const int index = findInode(path);
    if (index <= 0) return false;
    Inode& inode = inodes_[static_cast<std::size_t>(index)];
    if (!inode.isDirectory()) return false;
    inode = Inode{};
    inodeBitmap_.clear(static_cast<std::size_t>(index));
    if (!rootDirectory_.removeEntry(name)) return false;
    return persistMetadata();
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path) {
    std::vector<std::string> result;
    if (!mounted_ || normalizePath(path) != "/") return result;
    for (const auto& entry : rootDirectory_.getEntries()) result.push_back(entry.name);
    return result;
}

bool FileSystem::exists(const std::string& path) const {
    return mounted_ && findInode(path) >= 0;
}

void FileSystem::printInfo() const {
    std::cout << "===== Virtual File System =====\n"
              << "Block size: " << superblock_.blockSize << " bytes\n"
              << "Total blocks: " << superblock_.totalBlocks << "\n"
              << "Free blocks: " << blockBitmap_.countFree() << "\n"
              << "Total inodes: " << superblock_.totalInodes << "\n"
              << "Free inodes: " << inodeBitmap_.countFree() << "\n"
              << "Mounted: " << (mounted_ ? "yes" : "no") << "\n"
              << "===============================\n";
}

bool FileSystem::persistRootDirectory() {
    if (inodes_.empty() || inodes_[0].directBlocks[0] == 0) return false;
    DiskDirectoryBlock diskDirectory{};
    const auto& entries = rootDirectory_.getEntries();
    if (entries.size() > std::size(diskDirectory.entries)) return false;
    diskDirectory.count = static_cast<std::uint32_t>(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].name.size() >= DIRECTORY_NAME_BYTES) return false;
        diskDirectory.entries[i].inodeNumber = entries[i].inodeNumber;
        std::memcpy(diskDirectory.entries[i].name, entries[i].name.c_str(), entries[i].name.size() + 1);
    }
    std::vector<char> block(B, 0);
    std::memcpy(block.data(), &diskDirectory, sizeof(diskDirectory));
    return disk_.writeBlock(inodes_[0].directBlocks[0], block.data());
}

bool FileSystem::loadRootDirectory() {
    if (inodes_.empty() || inodes_[0].directBlocks[0] == 0) return false;
    std::vector<char> block(B, 0);
    if (!disk_.readBlock(inodes_[0].directBlocks[0], block.data())) return false;
    DiskDirectoryBlock diskDirectory{};
    std::memcpy(&diskDirectory, block.data(), sizeof(diskDirectory));
    if (diskDirectory.count > std::size(diskDirectory.entries)) return false;

    rootDirectory_ = Directory{};
    for (std::size_t i = 0; i < diskDirectory.count; ++i) {
        const auto& entry = diskDirectory.entries[i];
        const auto length = ::strnlen(entry.name, DIRECTORY_NAME_BYTES);
        if (length == 0 || length >= DIRECTORY_NAME_BYTES) return false;
        if (entry.inodeNumber >= inodes_.size() || !inodeBitmap_.isSet(entry.inodeNumber)) return false;
        if (!rootDirectory_.addEntry(std::string(entry.name, length), entry.inodeNumber)) return false;
    }
    return true;
}

bool FileSystem::persistMetadata() {
    if (inodes_.empty() || !superblock_.isValid()) return false;

    // The superblock tracks the current free counts.
    superblock_.freeBlocks = static_cast<std::uint32_t>(blockBitmap_.countFree());
    superblock_.freeInodes = static_cast<std::uint32_t>(inodeBitmap_.countFree());

    std::vector<unsigned char> block(B, 0);
    std::memcpy(block.data(), &superblock_, sizeof(superblock_));
    if (!disk_.writeBlock(0, block.data())) return false;

    // Store each bitmap as a compact bit-packed byte sequence in its own block.
    std::fill(block.begin(), block.end(), 0);
    for (std::size_t i = 0; i < inodeBitmap_.size(); ++i) {
        if (inodeBitmap_.isSet(i)) block[i / 8] |= static_cast<unsigned char>(1u << (i % 8));
    }
    if (inodeBitmap_.size() > B * 8 || !disk_.writeBlock(superblock_.inodeBitmapStart, block.data())) return false;

    std::fill(block.begin(), block.end(), 0);
    if (blockBitmap_.size() > B * 8) return false;
    for (std::size_t i = 0; i < blockBitmap_.size(); ++i) {
        if (blockBitmap_.isSet(i)) block[i / 8] |= static_cast<unsigned char>(1u << (i % 8));
    }
    if (!disk_.writeBlock(superblock_.blockBitmapStart, block.data())) return false;

    const std::size_t tableBytes = inodes_.size() * sizeof(Inode);
    if (!writeBytesToBlocks(disk_, superblock_.inodeTableStart,
                            superblock_.inodeTableBlocks, inodes_.data(), tableBytes)) return false;

    return persistRootDirectory();
}

bool FileSystem::loadMetadata() {
    std::vector<unsigned char> block(B, 0);
    if (!disk_.readBlock(0, block.data())) return false;
    std::memcpy(&superblock_, block.data(), sizeof(superblock_));
    if (!superblock_.isValid() || superblock_.totalInodes == 0 ||
        superblock_.totalBlocks == 0 || superblock_.inodeBitmapStart >= superblock_.totalBlocks ||
        superblock_.blockBitmapStart >= superblock_.totalBlocks ||
        superblock_.inodeTableStart + superblock_.inodeTableBlocks > superblock_.totalBlocks ||
        superblock_.totalInodes > (superblock_.inodeTableBlocks * B) / sizeof(Inode) ||
        superblock_.totalInodes > B * 8 || superblock_.totalBlocks > B * 8) return false;

    inodeBitmap_.initialize(superblock_.totalInodes);
    blockBitmap_.initialize(superblock_.totalBlocks);
    inodes_.assign(superblock_.totalInodes, Inode{});

    if (!disk_.readBlock(superblock_.inodeBitmapStart, block.data())) return false;
    for (std::size_t i = 0; i < inodeBitmap_.size(); ++i) {
        if ((block[i / 8] & static_cast<unsigned char>(1u << (i % 8))) != 0) inodeBitmap_.set(i);
    }

    if (!disk_.readBlock(superblock_.blockBitmapStart, block.data())) return false;
    for (std::size_t i = 0; i < blockBitmap_.size(); ++i) {
        if ((block[i / 8] & static_cast<unsigned char>(1u << (i % 8))) != 0) blockBitmap_.set(i);
    }

    if (!readBytesFromBlocks(disk_, superblock_.inodeTableStart,
                            superblock_.inodeTableBlocks, inodes_.data(),
                            inodes_.size() * sizeof(Inode))) return false;
    if (inodes_.empty() || !inodeBitmap_.isSet(0) || !inodes_[0].isDirectory()) return false;
    return loadRootDirectory();
}

