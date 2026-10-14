#ifndef SUPERBLOCK_HPP
#define SUPERBLOCK_HPP

#include <cstdint>

struct Superblock {
    static constexpr std::uint32_t MAGIC = 0x56534653; // "VSFS"

    std::uint32_t magic;
    std::uint32_t blockSize;
    std::uint32_t totalBlocks;

    std::uint32_t freeBlocks;
    std::uint32_t totalInodes;
    std::uint32_t freeInodes;

    std::uint32_t inodeBitmapStart;
    std::uint32_t blockBitmapStart;

    std::uint32_t inodeTableStart;
    std::uint32_t inodeTableBlocks;

    std::uint32_t dataBlockStart;

    void initialize(
        std::uint32_t totalBlockCount,
        std::uint32_t inodeCount
    );

    bool isValid() const;
};

#endif
