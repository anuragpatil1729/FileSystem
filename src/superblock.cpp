#include "superblock.hpp"

#include "byte_io.hpp"

#include <cstring>

namespace {
constexpr std::uint64_t BITS_PER_BLOCK =
    static_cast<std::uint64_t>(Superblock::BLOCK_SIZE) * 8;
constexpr std::uint64_t INODES_PER_BLOCK =
    Superblock::BLOCK_SIZE / Superblock::INODE_RECORD_SIZE;

std::uint64_t ceilDiv(std::uint64_t value, std::uint64_t divisor) {
    return (value + divisor - 1) / divisor;
}

bool fail(std::string* error, const std::string& message) {
    if (error) *error = message;
    return false;
}
} // namespace

bool Superblock::initialize(
    std::uint32_t totalBlockCount,
    std::uint32_t inodeCount,
    std::string* error
) {
    *this = Superblock{};

    if (totalBlockCount == 0 || totalBlockCount > MAX_TOTAL_BLOCKS) {
        return fail(error, "block count must be between 1 and " +
                               std::to_string(MAX_TOTAL_BLOCKS));
    }
    if (inodeCount == 0 || inodeCount > MAX_INODES) {
        return fail(error, "inode count must be between 1 and " +
                               std::to_string(MAX_INODES));
    }

    // 64-bit arithmetic: none of these sums can overflow.
    const std::uint64_t inodeBitmap = ceilDiv(inodeCount, BITS_PER_BLOCK);
    const std::uint64_t blockBitmap = ceilDiv(totalBlockCount, BITS_PER_BLOCK);
    const std::uint64_t inodeTable = ceilDiv(inodeCount, INODES_PER_BLOCK);
    const std::uint64_t dataStart = 1 + inodeBitmap + blockBitmap + inodeTable;

    if (dataStart >= totalBlockCount) {
        return fail(error, "metadata needs " + std::to_string(dataStart) +
                               " blocks, leaving no data blocks on a " +
                               std::to_string(totalBlockCount) + "-block disk");
    }

    magic = MAGIC;
    version = VERSION;
    blockSize = BLOCK_SIZE;
    totalBlocks = totalBlockCount;
    totalInodes = inodeCount;

    inodeBitmapStart = 1;
    inodeBitmapBlocks = static_cast<std::uint32_t>(inodeBitmap);
    blockBitmapStart = inodeBitmapStart + inodeBitmapBlocks;
    blockBitmapBlocks = static_cast<std::uint32_t>(blockBitmap);
    inodeTableStart = blockBitmapStart + blockBitmapBlocks;
    inodeTableBlocks = static_cast<std::uint32_t>(inodeTable);
    dataBlockStart = static_cast<std::uint32_t>(dataStart);

    // Metadata blocks are not free; the root inode is allocated by format.
    freeBlocks = totalBlocks - dataBlockStart;
    freeInodes = inodeCount;

    rootInode = ROOT_INODE;
    inodeRecordSize = INODE_RECORD_SIZE;
    directoryEntrySize = DIRECTORY_ENTRY_SIZE;
    return true;
}

bool Superblock::isValid() const {
    return magic == MAGIC &&
           version == VERSION &&
           blockSize == BLOCK_SIZE &&
           inodeRecordSize == INODE_RECORD_SIZE &&
           directoryEntrySize == DIRECTORY_ENTRY_SIZE &&
           totalBlocks != 0 &&
           totalInodes != 0;
}

bool Superblock::validateLayout(std::string* error) const {
    if (magic != MAGIC) {
        return fail(error, "bad superblock magic (not a VSFS image)");
    }
    if (version != VERSION) {
        return fail(error, "unsupported filesystem version " +
                               std::to_string(version) + " (expected " +
                               std::to_string(VERSION) + ")");
    }
    if (blockSize != BLOCK_SIZE) {
        return fail(error, "unsupported block size " + std::to_string(blockSize));
    }
    if (inodeRecordSize != INODE_RECORD_SIZE ||
        directoryEntrySize != DIRECTORY_ENTRY_SIZE) {
        return fail(error, "unsupported inode or directory record size");
    }

    Superblock expected;
    std::string reason;
    if (!expected.initialize(totalBlocks, totalInodes, &reason)) {
        return fail(error, "impossible geometry in superblock: " + reason);
    }

    if (inodeBitmapStart != expected.inodeBitmapStart ||
        inodeBitmapBlocks != expected.inodeBitmapBlocks ||
        blockBitmapStart != expected.blockBitmapStart ||
        blockBitmapBlocks != expected.blockBitmapBlocks ||
        inodeTableStart != expected.inodeTableStart ||
        inodeTableBlocks != expected.inodeTableBlocks ||
        dataBlockStart != expected.dataBlockStart) {
        return fail(error, "metadata region boundaries are inconsistent with geometry");
    }
    if (rootInode != ROOT_INODE) {
        return fail(error, "invalid root inode number");
    }
    if (freeBlocks > totalBlocks - dataBlockStart || freeInodes > totalInodes) {
        return fail(error, "free counts exceed capacity");
    }
    return true;
}

void Superblock::serialize(std::uint8_t* block) const {
    std::memset(block, 0, BLOCK_SIZE);
    const std::uint32_t fields[] = {
        magic, version, blockSize, totalBlocks, freeBlocks, totalInodes,
        freeInodes, inodeBitmapStart, inodeBitmapBlocks, blockBitmapStart,
        blockBitmapBlocks, inodeTableStart, inodeTableBlocks, dataBlockStart,
        rootInode, inodeRecordSize, directoryEntrySize
    };
    std::size_t offset = 0;
    for (const auto field : fields) {
        byteio::putU32(block + offset, field);
        offset += 4;
    }
}

Superblock Superblock::deserialize(const std::uint8_t* block) {
    Superblock sb{};
    std::uint32_t* fields[] = {
        &sb.magic, &sb.version, &sb.blockSize, &sb.totalBlocks, &sb.freeBlocks,
        &sb.totalInodes, &sb.freeInodes, &sb.inodeBitmapStart,
        &sb.inodeBitmapBlocks, &sb.blockBitmapStart, &sb.blockBitmapBlocks,
        &sb.inodeTableStart, &sb.inodeTableBlocks, &sb.dataBlockStart,
        &sb.rootInode, &sb.inodeRecordSize, &sb.directoryEntrySize
    };
    std::size_t offset = 0;
    for (auto* field : fields) {
        *field = byteio::getU32(block + offset);
        offset += 4;
    }
    return sb;
}
