#include "filesystem.hpp"
#include "byte_io.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace {
constexpr std::size_t B = Superblock::BLOCK_SIZE;

bool fail(std::string* err, const std::string& msg) {
    if (err) *err = msg;
    return false;
}
} // namespace

FileSystem::FileSystem(const std::string& diskPath)
    : disk_(diskPath),
      mounted_(false) {
}

void FileSystem::setError(const std::string& msg) const {
    lastError_ = msg;
}

const std::string& FileSystem::getLastError() const {
    return lastError_;
}

bool FileSystem::isMounted() const {
    return mounted_;
}

const Superblock& FileSystem::getSuperblock() const {
    return superblock_;
}

DiskManager& FileSystem::getDiskManager() {
    return disk_;
}

const DiskManager& FileSystem::getDiskManager() const {
    return disk_;
}

std::size_t FileSystem::getMaxFileSize() const {
    return Inode::MAX_FILE_SIZE;
}

std::vector<std::string> FileSystem::splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string token;
    for (char ch : path) {
        if (ch == '/') {
            if (!token.empty()) {
                parts.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    if (!token.empty()) {
        parts.push_back(token);
    }
    return parts;
}

bool FileSystem::resolveParentAndName(const std::string& path,
                                      std::uint32_t& parentInode,
                                      std::string& entryName) const {
    const auto parts = splitPath(path);
    if (parts.empty()) {
        setError("Cannot modify root directory itself");
        return false;
    }

    entryName = parts.back();
    if (entryName.empty() || entryName.size() > Directory::MAX_NAME_LENGTH) {
        setError("Invalid component name length");
        return false;
    }

    if (parts.size() == 1) {
        parentInode = Superblock::ROOT_INODE;
        return true;
    }

    std::uint32_t curr = Superblock::ROOT_INODE;
    for (std::size_t i = 0; i < parts.size() - 1; ++i) {
        if (curr >= inodes_.size() || !inodes_[curr].isDirectory()) {
            setError("Path component is not a directory");
            return false;
        }
        Directory dir;
        if (!loadDirectory(curr, dir)) {
            setError("Failed to load directory");
            return false;
        }
        const int next = dir.findEntry(parts[i]);
        if (next < 0) {
            setError("Directory component '" + parts[i] + "' not found");
            return false;
        }
        curr = static_cast<std::uint32_t>(next);
    }

    if (curr >= inodes_.size() || !inodes_[curr].isDirectory()) {
        setError("Parent component is not a directory");
        return false;
    }

    parentInode = curr;
    return true;
}

int FileSystem::resolvePath(const std::string& path) const {
    if (!mounted_) return -1;

    const auto parts = splitPath(path);
    if (parts.empty()) {
        return static_cast<int>(Superblock::ROOT_INODE);
    }

    std::uint32_t curr = Superblock::ROOT_INODE;
    for (const auto& part : parts) {
        if (curr >= inodes_.size() || !inodes_[curr].isDirectory()) {
            return -1;
        }
        Directory dir;
        if (!loadDirectory(curr, dir)) {
            return -1;
        }
        const int next = dir.findEntry(part);
        if (next < 0) {
            return -1;
        }
        curr = static_cast<std::uint32_t>(next);
    }

    return static_cast<int>(curr);
}

int FileSystem::findInode(const std::string& path) const {
    return resolvePath(path);
}

int FileSystem::allocateInode(InodeType type) {
    const std::size_t index = inodeBitmap_.findFree();
    if (index >= inodeBitmap_.size()) {
        setError("No free inodes available");
        return -1;
    }
    if (!inodeBitmap_.set(index)) {
        return -1;
    }
    inodes_[index].initialize(static_cast<std::uint32_t>(index), type);
    return static_cast<int>(index);
}

int FileSystem::allocateBlock() {
    for (std::size_t i = superblock_.dataBlockStart; i < blockBitmap_.size(); ++i) {
        if (blockBitmap_.isFree(i)) {
            if (!blockBitmap_.set(i)) {
                return -1;
            }
            return static_cast<int>(i);
        }
    }
    setError("No free data blocks available");
    return -1;
}

bool FileSystem::freeBlock(std::size_t blockNumber) {
    if (blockNumber < superblock_.dataBlockStart || blockNumber >= blockBitmap_.size()) {
        return false;
    }
    return blockBitmap_.clear(blockNumber);
}

void FileSystem::freeInodeBlocks(Inode& inode) {
    for (std::size_t i = 0; i < Inode::DIRECT_BLOCKS; ++i) {
        if (inode.directBlocks[i] != 0) {
            freeBlock(inode.directBlocks[i]);
            inode.directBlocks[i] = 0;
        }
    }

    if (inode.indirectBlock != 0) {
        std::vector<std::uint8_t> blockBuf(B, 0);
        if (disk_.readBlock(inode.indirectBlock, blockBuf.data())) {
            for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
                const std::uint32_t ptr = byteio::getU32(blockBuf.data() + p * 4);
                if (ptr != 0) {
                    freeBlock(ptr);
                }
            }
        }
        freeBlock(inode.indirectBlock);
        inode.indirectBlock = 0;
    }

    inode.size = 0;
}

bool FileSystem::format(std::size_t blockCount, std::uint32_t inodeCount) {
    Superblock sb;
    std::string err;
    if (!sb.initialize(static_cast<std::uint32_t>(blockCount), inodeCount, &err)) {
        setError("Format validation failed: " + err);
        return false;
    }

    if (!disk_.initialize(blockCount)) {
        setError("Failed to initialize virtual disk image");
        return false;
    }

    superblock_ = sb;
    inodeBitmap_.initialize(superblock_.totalInodes);
    blockBitmap_.initialize(superblock_.totalBlocks);
    inodes_.assign(superblock_.totalInodes, Inode{});

    // Mark metadata regions as allocated in block bitmap
    blockBitmap_.set(0); // Superblock
    for (std::size_t i = 0; i < superblock_.inodeBitmapBlocks; ++i) {
        blockBitmap_.set(superblock_.inodeBitmapStart + i);
    }
    for (std::size_t i = 0; i < superblock_.blockBitmapBlocks; ++i) {
        blockBitmap_.set(superblock_.blockBitmapStart + i);
    }
    for (std::size_t i = 0; i < superblock_.inodeTableBlocks; ++i) {
        blockBitmap_.set(superblock_.inodeTableStart + i);
    }

    // Allocate root directory at inode 0
    const int rootIdx = allocateInode(InodeType::DIRECTORY);
    if (rootIdx != static_cast<int>(Superblock::ROOT_INODE)) {
        setError("Failed to allocate root inode");
        return false;
    }

    mounted_ = true;
    if (!persistMetadata()) {
        mounted_ = false;
        setError("Failed to persist initial metadata");
        return false;
    }

    return true;
}

bool FileSystem::mount() {
    if (mounted_) return true;
    if (!loadMetadata()) {
        mounted_ = false;
        return false;
    }
    mounted_ = true;
    return true;
}

bool FileSystem::unmount() {
    if (!mounted_) return true;
    const bool ok = persistMetadata();
    mounted_ = false;
    return ok;
}

bool FileSystem::readInodeData(std::uint32_t inodeNumber,
                              std::vector<std::uint8_t>& out) const {
    if (inodeNumber >= inodes_.size()) {
        setError("Invalid inode number");
        return false;
    }

    const Inode& inode = inodes_[inodeNumber];
    out.resize(inode.size);
    if (inode.size == 0) {
        return true;
    }

    const std::size_t numBlocks = (inode.size + B - 1) / B;
    if (numBlocks > Inode::MAX_DATA_BLOCKS) {
        setError("Inode size exceeds maximum supported blocks");
        return false;
    }

    std::vector<std::uint32_t> indirectTable(Inode::POINTERS_PER_INDIRECT, 0);
    if (numBlocks > Inode::DIRECT_BLOCKS) {
        if (inode.indirectBlock == 0) {
            setError("Missing indirect block for multi-block inode");
            return false;
        }
        std::vector<std::uint8_t> buf(B, 0);
        if (!disk_.readBlock(inode.indirectBlock, buf.data())) {
            setError("Failed to read indirect block");
            return false;
        }
        for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
            indirectTable[p] = byteio::getU32(buf.data() + p * 4);
        }
    }

    for (std::size_t k = 0; k < numBlocks; ++k) {
        const std::uint32_t phys = (k < Inode::DIRECT_BLOCKS)
                                       ? inode.directBlocks[k]
                                       : indirectTable[k - Inode::DIRECT_BLOCKS];
        const std::size_t offset = k * B;
        const std::size_t bytesToCopy = std::min(B, inode.size - offset);

        if (phys == 0) {
            std::memset(out.data() + offset, 0, bytesToCopy);
        } else {
            std::vector<std::uint8_t> blockBuf(B, 0);
            if (!disk_.readBlock(phys, blockBuf.data())) {
                setError("Failed to read data block " + std::to_string(phys));
                return false;
            }
            std::memcpy(out.data() + offset, blockBuf.data(), bytesToCopy);
        }
    }

    return true;
}

bool FileSystem::writeInodeData(std::uint32_t inodeNumber,
                               const void* data,
                               std::size_t size) {
    if (inodeNumber >= inodes_.size()) {
        setError("Invalid inode number");
        return false;
    }
    if (size > Inode::MAX_FILE_SIZE) {
        setError("Write size exceeds maximum supported file size (" +
                 std::to_string(Inode::MAX_FILE_SIZE) + " bytes)");
        return false;
    }

    Inode& inode = inodes_[inodeNumber];
    const std::size_t oldBlocks = (inode.size + B - 1) / B;
    const std::size_t newBlocks = (size + B - 1) / B;
    const bool needIndirect = (newBlocks > Inode::DIRECT_BLOCKS);
    const bool hadIndirect = (inode.indirectBlock != 0);

    // Calculate additional blocks required
    const std::size_t addData = (newBlocks > oldBlocks) ? (newBlocks - oldBlocks) : 0;
    const std::size_t addInd = (needIndirect && !hadIndirect) ? 1 : 0;
    const std::size_t totalNeeded = addData + addInd;

    if (blockBitmap_.countFree() < totalNeeded) {
        setError("Disk space exhausted: cannot allocate " +
                 std::to_string(totalNeeded) + " blocks");
        return false;
    }

    // Preserve original block state in case rollback is needed
    const Inode origInode = inode;
    std::vector<std::size_t> allocatedBlocks;
    auto allocNew = [&]() -> int {
        const int b = allocateBlock();
        if (b >= 0) {
            allocatedBlocks.push_back(static_cast<std::size_t>(b));
        }
        return b;
    };

    auto rollback = [&]() {
        for (std::size_t b : allocatedBlocks) {
            freeBlock(b);
        }
        inode = origInode;
    };

    std::vector<std::uint32_t> indirectTable(Inode::POINTERS_PER_INDIRECT, 0);
    if (hadIndirect) {
        std::vector<std::uint8_t> buf(B, 0);
        if (disk_.readBlock(inode.indirectBlock, buf.data())) {
            for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
                indirectTable[p] = byteio::getU32(buf.data() + p * 4);
            }
        }
    }

    if (needIndirect && !hadIndirect) {
        const int indBlock = allocNew();
        if (indBlock < 0) {
            rollback();
            return false;
        }
        inode.indirectBlock = static_cast<std::uint32_t>(indBlock);
    }

    const auto* src = static_cast<const std::uint8_t*>(data);

    for (std::size_t k = 0; k < newBlocks; ++k) {
        std::uint32_t phys = 0;
        if (k < Inode::DIRECT_BLOCKS) {
            if (inode.directBlocks[k] == 0) {
                const int nb = allocNew();
                if (nb < 0) {
                    rollback();
                    return false;
                }
                inode.directBlocks[k] = static_cast<std::uint32_t>(nb);
            }
            phys = inode.directBlocks[k];
        } else {
            const std::size_t idx = k - Inode::DIRECT_BLOCKS;
            if (indirectTable[idx] == 0) {
                const int nb = allocNew();
                if (nb < 0) {
                    rollback();
                    return false;
                }
                indirectTable[idx] = static_cast<std::uint32_t>(nb);
            }
            phys = indirectTable[idx];
        }

        const std::size_t offset = k * B;
        const std::size_t bytesToWrite = std::min(B, size - offset);
        std::vector<std::uint8_t> blockBuf(B, 0);
        if (bytesToWrite > 0 && src != nullptr) {
            std::memcpy(blockBuf.data(), src + offset, bytesToWrite);
        }

        if (!disk_.writeBlock(phys, blockBuf.data())) {
            setError("Disk write failed for block " + std::to_string(phys));
            rollback();
            return false;
        }
    }

    if (needIndirect) {
        std::vector<std::uint8_t> buf(B, 0);
        for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
            byteio::putU32(buf.data() + p * 4, indirectTable[p]);
        }
        if (!disk_.writeBlock(inode.indirectBlock, buf.data())) {
            setError("Disk write failed for indirect block");
            rollback();
            return false;
        }
    }

    // Free truncated blocks
    if (newBlocks < oldBlocks) {
        if (hadIndirect) {
            for (std::size_t k = std::max<std::size_t>(Inode::DIRECT_BLOCKS, newBlocks);
                 k < oldBlocks; ++k) {
                const std::size_t idx = k - Inode::DIRECT_BLOCKS;
                if (idx < Inode::POINTERS_PER_INDIRECT && indirectTable[idx] != 0) {
                    freeBlock(indirectTable[idx]);
                    indirectTable[idx] = 0;
                }
            }
            if (!needIndirect && inode.indirectBlock != 0) {
                freeBlock(inode.indirectBlock);
                inode.indirectBlock = 0;
            } else if (needIndirect) {
                std::vector<std::uint8_t> buf(B, 0);
                for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
                    byteio::putU32(buf.data() + p * 4, indirectTable[p]);
                }
                disk_.writeBlock(inode.indirectBlock, buf.data());
            }
        }
        for (std::size_t k = newBlocks; k < std::min<std::size_t>(Inode::DIRECT_BLOCKS, oldBlocks); ++k) {
            if (inode.directBlocks[k] != 0) {
                freeBlock(inode.directBlocks[k]);
                inode.directBlocks[k] = 0;
            }
        }
    }

    inode.size = static_cast<std::uint32_t>(size);
    inode.touch();

    if (!persistMetadata()) {
        setError("Failed to persist metadata after write");
        return false;
    }

    return true;
}

bool FileSystem::loadDirectory(std::uint32_t inodeNumber, Directory& dir) const {
    std::vector<std::uint8_t> data;
    if (!readInodeData(inodeNumber, data)) {
        return false;
    }
    return dir.deserialize(data.data(), data.size());
}

bool FileSystem::saveDirectory(std::uint32_t inodeNumber, const Directory& dir) {
    const auto data = dir.serialize();
    return writeInodeData(inodeNumber, data.data(), data.size());
}

bool FileSystem::createFile(const std::string& path) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return false;
    }

    std::uint32_t parentInode = 0;
    std::string name;
    if (!resolveParentAndName(path, parentInode, name)) {
        return false;
    }

    Directory parentDir;
    if (!loadDirectory(parentInode, parentDir)) {
        setError("Failed to load parent directory");
        return false;
    }

    if (parentDir.exists(name)) {
        setError("File or directory '" + name + "' already exists");
        return false;
    }

    const int newInodeIdx = allocateInode(InodeType::FILE);
    if (newInodeIdx < 0) {
        return false;
    }

    if (!parentDir.addEntry(name, static_cast<std::uint32_t>(newInodeIdx))) {
        inodeBitmap_.clear(static_cast<std::size_t>(newInodeIdx));
        inodes_[static_cast<std::size_t>(newInodeIdx)] = Inode{};
        setError("Failed to add directory entry");
        return false;
    }

    if (!saveDirectory(parentInode, parentDir)) {
        parentDir.removeEntry(name);
        inodeBitmap_.clear(static_cast<std::size_t>(newInodeIdx));
        inodes_[static_cast<std::size_t>(newInodeIdx)] = Inode{};
        setError("Failed to save parent directory");
        return false;
    }

    return true;
}

bool FileSystem::createDirectory(const std::string& path) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return false;
    }

    std::uint32_t parentInode = 0;
    std::string name;
    if (!resolveParentAndName(path, parentInode, name)) {
        return false;
    }

    Directory parentDir;
    if (!loadDirectory(parentInode, parentDir)) {
        setError("Failed to load parent directory");
        return false;
    }

    if (parentDir.exists(name)) {
        setError("File or directory '" + name + "' already exists");
        return false;
    }

    const int newInodeIdx = allocateInode(InodeType::DIRECTORY);
    if (newInodeIdx < 0) {
        return false;
    }

    if (!parentDir.addEntry(name, static_cast<std::uint32_t>(newInodeIdx))) {
        inodeBitmap_.clear(static_cast<std::size_t>(newInodeIdx));
        inodes_[static_cast<std::size_t>(newInodeIdx)] = Inode{};
        setError("Failed to add directory entry");
        return false;
    }

    if (!saveDirectory(parentInode, parentDir)) {
        parentDir.removeEntry(name);
        inodeBitmap_.clear(static_cast<std::size_t>(newInodeIdx));
        inodes_[static_cast<std::size_t>(newInodeIdx)] = Inode{};
        setError("Failed to save parent directory");
        return false;
    }

    return true;
}

bool FileSystem::writeFile(const std::string& path, const std::string& data) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return false;
    }

    const int inodeIdx = resolvePath(path);
    if (inodeIdx < 0) {
        setError("File not found: " + path);
        return false;
    }

    if (!inodes_[static_cast<std::size_t>(inodeIdx)].isFile()) {
        setError("Target is not a regular file: " + path);
        return false;
    }

    return writeInodeData(static_cast<std::uint32_t>(inodeIdx),
                          data.data(),
                          data.size());
}

std::string FileSystem::readFile(const std::string& path) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return {};
    }

    const int inodeIdx = resolvePath(path);
    if (inodeIdx < 0) {
        setError("File not found: " + path);
        return {};
    }

    if (!inodes_[static_cast<std::size_t>(inodeIdx)].isFile()) {
        setError("Target is not a regular file: " + path);
        return {};
    }

    std::vector<std::uint8_t> data;
    if (!readInodeData(static_cast<std::uint32_t>(inodeIdx), data)) {
        return {};
    }

    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

bool FileSystem::deleteFile(const std::string& path) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return false;
    }

    std::uint32_t parentInode = 0;
    std::string name;
    if (!resolveParentAndName(path, parentInode, name)) {
        return false;
    }

    Directory parentDir;
    if (!loadDirectory(parentInode, parentDir)) {
        setError("Failed to load parent directory");
        return false;
    }

    const int targetIdx = parentDir.findEntry(name);
    if (targetIdx < 0) {
        setError("File '" + name + "' not found");
        return false;
    }

    Inode& targetInode = inodes_[static_cast<std::size_t>(targetIdx)];
    if (!targetInode.isFile()) {
        setError("'" + name + "' is a directory, use rmdir");
        return false;
    }

    freeInodeBlocks(targetInode);
    targetInode = Inode{};
    inodeBitmap_.clear(static_cast<std::size_t>(targetIdx));

    parentDir.removeEntry(name);
    if (!saveDirectory(parentInode, parentDir)) {
        setError("Failed to update parent directory after deleting file");
        return false;
    }

    return true;
}

bool FileSystem::deleteDirectory(const std::string& path) {
    if (!mounted_) {
        setError("Filesystem not mounted");
        return false;
    }

    const auto parts = splitPath(path);
    if (parts.empty()) {
        setError("Cannot delete root directory");
        return false;
    }

    std::uint32_t parentInode = 0;
    std::string name;
    if (!resolveParentAndName(path, parentInode, name)) {
        return false;
    }

    Directory parentDir;
    if (!loadDirectory(parentInode, parentDir)) {
        setError("Failed to load parent directory");
        return false;
    }

    const int targetIdx = parentDir.findEntry(name);
    if (targetIdx < 0) {
        setError("Directory '" + name + "' not found");
        return false;
    }

    Inode& targetInode = inodes_[static_cast<std::size_t>(targetIdx)];
    if (!targetInode.isDirectory()) {
        setError("'" + name + "' is not a directory");
        return false;
    }

    Directory targetDir;
    if (!loadDirectory(static_cast<std::uint32_t>(targetIdx), targetDir)) {
        setError("Failed to inspect target directory contents");
        return false;
    }

    if (!targetDir.empty()) {
        setError("Cannot delete non-empty directory '" + name + "'");
        return false;
    }

    freeInodeBlocks(targetInode);
    targetInode = Inode{};
    inodeBitmap_.clear(static_cast<std::size_t>(targetIdx));

    parentDir.removeEntry(name);
    if (!saveDirectory(parentInode, parentDir)) {
        setError("Failed to update parent directory after deleting directory");
        return false;
    }

    return true;
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path) {
    std::vector<std::string> result;
    if (!mounted_) {
        setError("Filesystem not mounted");
        return result;
    }

    const int inodeIdx = resolvePath(path);
    if (inodeIdx < 0) {
        setError("Directory not found: " + path);
        return result;
    }

    if (!inodes_[static_cast<std::size_t>(inodeIdx)].isDirectory()) {
        setError("Path is not a directory: " + path);
        return result;
    }

    Directory dir;
    if (!loadDirectory(static_cast<std::uint32_t>(inodeIdx), dir)) {
        setError("Failed to load directory");
        return result;
    }

    for (const auto& entry : dir.getEntries()) {
        result.push_back(entry.name);
    }
    return result;
}

bool FileSystem::exists(const std::string& path) const {
    return mounted_ && resolvePath(path) >= 0;
}

void FileSystem::printInfo() const {
    std::cout << "===== Virtual File System =====\n"
              << "Block size: " << superblock_.blockSize << " bytes\n"
              << "Total blocks: " << superblock_.totalBlocks << "\n"
              << "Free blocks: " << blockBitmap_.countFree() << "\n"
              << "Total inodes: " << superblock_.totalInodes << "\n"
              << "Free inodes: " << inodeBitmap_.countFree() << "\n"
              << "Data block start: " << superblock_.dataBlockStart << "\n"
              << "Max file size: " << Inode::MAX_FILE_SIZE << " bytes ("
              << (Inode::MAX_FILE_SIZE / 1024) << " KiB)\n"
              << "Mounted: " << (mounted_ ? "yes" : "no") << "\n"
              << "===============================\n";
}

bool FileSystem::persistMetadata() {
    if (!superblock_.isValid()) {
        setError("Superblock is not valid");
        return false;
    }

    superblock_.freeBlocks = static_cast<std::uint32_t>(blockBitmap_.countFree());
    superblock_.freeInodes = static_cast<std::uint32_t>(inodeBitmap_.countFree());

    // 1. Superblock at block 0
    std::vector<std::uint8_t> block(B, 0);
    superblock_.serialize(block.data());
    if (!disk_.writeBlock(0, block.data())) {
        setError("Failed to write superblock to disk");
        return false;
    }

    // 2. Inode Bitmap
    const auto& inodeBits = inodeBitmap_.getRawBits();
    std::vector<std::uint8_t> inodeBitmapBuf(superblock_.inodeBitmapBlocks * B, 0);
    if (!inodeBits.empty()) {
        std::memcpy(inodeBitmapBuf.data(), inodeBits.data(), inodeBits.size());
    }
    for (std::size_t i = 0; i < superblock_.inodeBitmapBlocks; ++i) {
        if (!disk_.writeBlock(superblock_.inodeBitmapStart + i,
                              inodeBitmapBuf.data() + i * B)) {
            setError("Failed to write inode bitmap block " + std::to_string(i));
            return false;
        }
    }

    // 3. Block Bitmap
    const auto& blockBits = blockBitmap_.getRawBits();
    std::vector<std::uint8_t> blockBitmapBuf(superblock_.blockBitmapBlocks * B, 0);
    if (!blockBits.empty()) {
        std::memcpy(blockBitmapBuf.data(), blockBits.data(), blockBits.size());
    }
    for (std::size_t i = 0; i < superblock_.blockBitmapBlocks; ++i) {
        if (!disk_.writeBlock(superblock_.blockBitmapStart + i,
                              blockBitmapBuf.data() + i * B)) {
            setError("Failed to write block bitmap block " + std::to_string(i));
            return false;
        }
    }

    // 4. Inode Table (32 inodes per block)
    constexpr std::size_t INODES_PER_BLOCK = B / Inode::DISK_RECORD_SIZE;
    for (std::size_t b = 0; b < superblock_.inodeTableBlocks; ++b) {
        std::vector<std::uint8_t> tableBuf(B, 0);
        for (std::size_t j = 0; j < INODES_PER_BLOCK; ++j) {
            const std::size_t idx = b * INODES_PER_BLOCK + j;
            if (idx < inodes_.size()) {
                inodes_[idx].serialize(tableBuf.data() + j * Inode::DISK_RECORD_SIZE);
            }
        }
        if (!disk_.writeBlock(superblock_.inodeTableStart + b, tableBuf.data())) {
            setError("Failed to write inode table block " + std::to_string(b));
            return false;
        }
    }

    return true;
}

bool FileSystem::loadMetadata() {
    std::string err;
    if (!disk_.attachExisting(&err)) {
        setError("Cannot attach disk: " + err);
        return false;
    }

    std::vector<std::uint8_t> block(B, 0);
    if (!disk_.readBlock(0, block.data())) {
        setError("Failed to read superblock from disk");
        return false;
    }

    superblock_ = Superblock::deserialize(block.data());
    if (!superblock_.validateLayout(&err)) {
        setError("Corrupt superblock layout: " + err);
        return false;
    }

    if (superblock_.totalBlocks != disk_.getBlockCount()) {
        setError("Disk file size (" + std::to_string(disk_.getBlockCount()) +
                 " blocks) does not match superblock totalBlocks (" +
                 std::to_string(superblock_.totalBlocks) + ")");
        return false;
    }

    // Load Inode Bitmap
    std::vector<std::uint8_t> inodeBitmapBuf(superblock_.inodeBitmapBlocks * B, 0);
    for (std::size_t i = 0; i < superblock_.inodeBitmapBlocks; ++i) {
        if (!disk_.readBlock(superblock_.inodeBitmapStart + i,
                             inodeBitmapBuf.data() + i * B)) {
            setError("Failed to read inode bitmap block " + std::to_string(i));
            return false;
        }
    }
    inodeBitmap_.initialize(superblock_.totalInodes);
    inodeBitmap_.loadBits(inodeBitmapBuf.data(), inodeBitmapBuf.size());

    // Load Block Bitmap
    std::vector<std::uint8_t> blockBitmapBuf(superblock_.blockBitmapBlocks * B, 0);
    for (std::size_t i = 0; i < superblock_.blockBitmapBlocks; ++i) {
        if (!disk_.readBlock(superblock_.blockBitmapStart + i,
                             blockBitmapBuf.data() + i * B)) {
            setError("Failed to read block bitmap block " + std::to_string(i));
            return false;
        }
    }
    blockBitmap_.initialize(superblock_.totalBlocks);
    blockBitmap_.loadBits(blockBitmapBuf.data(), blockBitmapBuf.size());

    // Load Inode Table
    constexpr std::size_t INODES_PER_BLOCK = B / Inode::DISK_RECORD_SIZE;
    inodes_.assign(superblock_.totalInodes, Inode{});
    for (std::size_t b = 0; b < superblock_.inodeTableBlocks; ++b) {
        std::vector<std::uint8_t> tableBuf(B, 0);
        if (!disk_.readBlock(superblock_.inodeTableStart + b, tableBuf.data())) {
            setError("Failed to read inode table block " + std::to_string(b));
            return false;
        }
        for (std::size_t j = 0; j < INODES_PER_BLOCK; ++j) {
            const std::size_t idx = b * INODES_PER_BLOCK + j;
            if (idx < inodes_.size()) {
                inodes_[idx] = Inode::deserialize(tableBuf.data() + j * Inode::DISK_RECORD_SIZE);
            }
        }
    }

    // Comprehensive consistency audit
    if (!validateMetadataConsistency(&err)) {
        setError("Filesystem metadata consistency check failed: " + err);
        return false;
    }

    return true;
}

bool FileSystem::validateMetadataConsistency(std::string* error) const {
    // 1. Root inode must be allocated and must be a DIRECTORY
    if (!inodeBitmap_.isSet(Superblock::ROOT_INODE)) {
        return fail(error, "Root inode 0 is not marked allocated in inode bitmap");
    }
    if (!inodes_[Superblock::ROOT_INODE].isDirectory()) {
        return fail(error, "Root inode 0 is not a directory");
    }

    // 2. Metadata blocks must be marked allocated in block bitmap
    if (!blockBitmap_.isSet(0)) {
        return fail(error, "Superblock block 0 is not marked allocated");
    }
    for (std::size_t i = 0; i < superblock_.inodeBitmapBlocks; ++i) {
        if (!blockBitmap_.isSet(superblock_.inodeBitmapStart + i)) {
            return fail(error, "Inode bitmap block is marked free in block bitmap");
        }
    }
    for (std::size_t i = 0; i < superblock_.blockBitmapBlocks; ++i) {
        if (!blockBitmap_.isSet(superblock_.blockBitmapStart + i)) {
            return fail(error, "Block bitmap block is marked free in block bitmap");
        }
    }
    for (std::size_t i = 0; i < superblock_.inodeTableBlocks; ++i) {
        if (!blockBitmap_.isSet(superblock_.inodeTableStart + i)) {
            return fail(error, "Inode table block is marked free in block bitmap");
        }
    }

    // 3. Track reference counts for all data blocks to detect double allocation or leaks
    std::vector<std::uint32_t> blockRefs(superblock_.totalBlocks, 0);

    // Mark metadata blocks
    blockRefs[0] = 1;
    for (std::size_t i = 0; i < superblock_.inodeBitmapBlocks; ++i) {
        blockRefs[superblock_.inodeBitmapStart + i] = 1;
    }
    for (std::size_t i = 0; i < superblock_.blockBitmapBlocks; ++i) {
        blockRefs[superblock_.blockBitmapStart + i] = 1;
    }
    for (std::size_t i = 0; i < superblock_.inodeTableBlocks; ++i) {
        blockRefs[superblock_.inodeTableStart + i] = 1;
    }

    // 4. Inode table vs inode bitmap
    for (std::size_t i = 0; i < inodes_.size(); ++i) {
        const Inode& in = inodes_[i];
        if (inodeBitmap_.isSet(i)) {
            if (in.isFree()) {
                return fail(error, "Inode " + std::to_string(i) +
                                       " is marked allocated in bitmap but is FREE in table");
            }
            if (in.inodeNumber != i) {
                return fail(error, "Inode " + std::to_string(i) +
                                       " has mismatched number " + std::to_string(in.inodeNumber));
            }

            // Check direct block pointers
            for (std::size_t d = 0; d < Inode::DIRECT_BLOCKS; ++d) {
                const std::uint32_t blk = in.directBlocks[d];
                if (blk != 0) {
                    if (blk < superblock_.dataBlockStart || blk >= superblock_.totalBlocks) {
                        return fail(error, "Inode " + std::to_string(i) +
                                               " direct block pointer " + std::to_string(blk) +
                                               " out of data range");
                    }
                    if (!blockBitmap_.isSet(blk)) {
                        return fail(error, "Inode " + std::to_string(i) +
                                               " references block " + std::to_string(blk) +
                                               " marked free in block bitmap");
                    }
                    if (blockRefs[blk] > 0) {
                        return fail(error, "Duplicate block allocation: block " +
                                               std::to_string(blk) + " referenced multiple times");
                    }
                    blockRefs[blk] = 1;
                }
            }

            // Check indirect block
            if (in.indirectBlock != 0) {
                const std::uint32_t indBlk = in.indirectBlock;
                if (indBlk < superblock_.dataBlockStart || indBlk >= superblock_.totalBlocks) {
                    return fail(error, "Inode " + std::to_string(i) +
                                           " indirect block pointer " + std::to_string(indBlk) +
                                           " out of range");
                }
                if (!blockBitmap_.isSet(indBlk)) {
                    return fail(error, "Inode " + std::to_string(i) +
                                           " indirect block " + std::to_string(indBlk) +
                                           " marked free in block bitmap");
                }
                if (blockRefs[indBlk] > 0) {
                    return fail(error, "Duplicate block allocation for indirect block " +
                                           std::to_string(indBlk));
                }
                blockRefs[indBlk] = 1;

                std::vector<std::uint8_t> indBuf(B, 0);
                if (!disk_.readBlock(indBlk, indBuf.data())) {
                    return fail(error, "Cannot read indirect block " + std::to_string(indBlk));
                }
                for (std::size_t p = 0; p < Inode::POINTERS_PER_INDIRECT; ++p) {
                    const std::uint32_t ptr = byteio::getU32(indBuf.data() + p * 4);
                    if (ptr != 0) {
                        if (ptr < superblock_.dataBlockStart || ptr >= superblock_.totalBlocks) {
                            return fail(error, "Indirect entry references out of range block " +
                                                   std::to_string(ptr));
                        }
                        if (!blockBitmap_.isSet(ptr)) {
                            return fail(error, "Indirect block references block " +
                                                   std::to_string(ptr) +
                                                   " marked free in block bitmap");
                        }
                        if (blockRefs[ptr] > 0) {
                            return fail(error, "Duplicate block allocation in indirect table: " +
                                                   std::to_string(ptr));
                        }
                        blockRefs[ptr] = 1;
                    }
                }
            }

            // Check directory entries if this is a directory
            if (in.isDirectory()) {
                Directory dir;
                if (!loadDirectory(static_cast<std::uint32_t>(i), dir)) {
                    return fail(error, "Directory inode " + std::to_string(i) +
                                           " has corrupt directory data");
                }
                for (const auto& entry : dir.getEntries()) {
                    if (entry.inodeNumber >= inodes_.size()) {
                        return fail(error, "Directory entry '" + entry.name +
                                               "' points to out-of-range inode " +
                                               std::to_string(entry.inodeNumber));
                    }
                    if (!inodeBitmap_.isSet(entry.inodeNumber)) {
                        return fail(error, "Directory entry '" + entry.name +
                                               "' points to unallocated inode " +
                                               std::to_string(entry.inodeNumber));
                    }
                }
            }
        } else {
            // Unallocated inode must be clean
            if (!in.isFree()) {
                return fail(error, "Inode " + std::to_string(i) +
                                       " is marked FREE in bitmap but not FREE in table");
            }
        }
    }

    // 5. Block bitmap consistency with block references (leak check)
    for (std::size_t b = superblock_.dataBlockStart; b < superblock_.totalBlocks; ++b) {
        if (blockBitmap_.isSet(b) && blockRefs[b] == 0) {
            return fail(error, "Block " + std::to_string(b) +
                                   " is marked allocated in bitmap but is orphaned/unreferenced");
        }
        if (!blockBitmap_.isSet(b) && blockRefs[b] != 0) {
            return fail(error, "Block " + std::to_string(b) +
                                   " is unallocated in bitmap but referenced by an inode");
        }
    }

    return true;
}
