#include "superblock.hpp"

void Superblock::initialize(
    std::uint32_t totalBlockCount,
    std::uint32_t inodeCount
) {
    magic = MAGIC;

    blockSize = 4096;

    totalBlocks = totalBlockCount;

    totalInodes = inodeCount;

    /*
     * Initially everything except metadata
     * will be considered available.
     */
    freeBlocks = totalBlockCount;
    freeInodes = inodeCount;

    /*
     * Block 0:
     * Superblock
     */
    inodeBitmapStart = 1;

    /*
     * Block 2:
     * Block bitmap
     */
    blockBitmapStart = 2;

    /*
     * Inode table starts at block 3.
     */
    inodeTableStart = 3;

    /*
     * Calculate how many blocks
     * the inode table requires.
     *
     * This assumes one inode is roughly
     * a few dozen bytes.
     */
    constexpr std::uint32_t INODES_PER_BLOCK = 32;

    inodeTableBlocks =
        (inodeCount + INODES_PER_BLOCK - 1)
        / INODES_PER_BLOCK;

    dataBlockStart =
        inodeTableStart + inodeTableBlocks;

    /*
     * Metadata blocks are not free.
     */
    if (dataBlockStart < totalBlocks) {
        freeBlocks =
            totalBlocks - dataBlockStart;
    } else {
        freeBlocks = 0;
    }
}

bool Superblock::isValid() const {
    return magic == MAGIC &&
           blockSize != 0 &&
           totalBlocks != 0 &&
           totalInodes != 0;
}
