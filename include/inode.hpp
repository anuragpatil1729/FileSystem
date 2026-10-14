#ifndef INODE_HPP
#define INODE_HPP

#include <cstdint>
#include <cstddef>
enum class InodeType : std::uint8_t {
    FREE = 0,
    FILE = 1,
    DIRECTORY = 2
};

struct Inode {
    static constexpr std::size_t DIRECT_BLOCKS = 12;

    std::uint32_t inodeNumber;

    InodeType type;

    std::uint32_t size;

    std::uint32_t owner;

    std::uint32_t permissions;

    std::uint32_t linkCount;

    std::uint32_t directBlocks[DIRECT_BLOCKS];

    std::uint32_t indirectBlock;

    std::uint64_t createdAt;

    std::uint64_t modifiedAt;

    bool isFree() const;

    bool isFile() const;

    bool isDirectory() const;

    void initialize(
        std::uint32_t number,
        InodeType inodeType
    );
};

#endif
