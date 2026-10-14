#ifndef SUPERBLOCK_HPP
#define SUPERBLOCK_HPP

#include <cstdint>
#include <string>

/*
 * On-disk layout (format version 2), in block units:
 *
 *   [0]                       superblock
 *   [inodeBitmapStart ...]    inode bitmap   (inodeBitmapBlocks)
 *   [blockBitmapStart ...]    block bitmap   (blockBitmapBlocks)
 *   [inodeTableStart ...]     inode table    (inodeTableBlocks)
 *   [dataBlockStart ...]      data blocks    (to totalBlocks - 1)
 *
 * The layout is a pure function of (totalBlocks, totalInodes). On mount
 * the stored region fields must match the recomputed layout exactly,
 * which rejects overlapping or out-of-bounds regions.
 */
struct Superblock {
    static constexpr std::uint32_t MAGIC = 0x56534653; // "VSFS"
    static constexpr std::uint32_t VERSION = 2;
    static constexpr std::uint32_t BLOCK_SIZE = 4096;
    static constexpr std::uint32_t INODE_RECORD_SIZE = 128;
    static constexpr std::uint32_t DIRECTORY_ENTRY_SIZE = 128;
    static constexpr std::uint32_t ROOT_INODE = 0;
    static constexpr std::uint32_t MAX_TOTAL_BLOCKS = 1u << 20;  // 4 GiB
    static constexpr std::uint32_t MAX_INODES = 1u << 20;

    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t blockSize;
    std::uint32_t totalBlocks;

    std::uint32_t freeBlocks;
    std::uint32_t totalInodes;
    std::uint32_t freeInodes;

    std::uint32_t inodeBitmapStart;
    std::uint32_t inodeBitmapBlocks;
    std::uint32_t blockBitmapStart;
    std::uint32_t blockBitmapBlocks;

    std::uint32_t inodeTableStart;
    std::uint32_t inodeTableBlocks;

    std::uint32_t dataBlockStart;

    std::uint32_t rootInode;
    std::uint32_t inodeRecordSize;
    std::uint32_t directoryEntrySize;

    /*
     * Computes the layout for the requested geometry. Returns false (and
     * leaves a reason in *error) if the geometry is impossible, e.g. the
     * metadata would not leave at least one data block.
     */
    bool initialize(
        std::uint32_t totalBlockCount,
        std::uint32_t inodeCount,
        std::string* error = nullptr
    );

    // Magic, version, and constant format parameters.
    bool isValid() const;

    // Full structural validation against the recomputed layout.
    bool validateLayout(std::string* error = nullptr) const;

    void serialize(std::uint8_t* block) const;

    static Superblock deserialize(const std::uint8_t* block);
};

#endif
