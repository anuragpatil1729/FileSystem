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

    // One indirect block holds this many 32-bit block pointers.
    static constexpr std::size_t POINTERS_PER_INDIRECT = 4096 / sizeof(std::uint32_t);

    // 12 direct + 1024 single-indirect = 1036 data blocks per inode.
    static constexpr std::size_t MAX_DATA_BLOCKS = DIRECT_BLOCKS + POINTERS_PER_INDIRECT;

    // 1036 * 4096 = 4,243,456 bytes.
    static constexpr std::size_t MAX_FILE_SIZE = MAX_DATA_BLOCKS * 4096;

    // Size of one serialized inode record in the inode table.
    static constexpr std::size_t DISK_RECORD_SIZE = 128;

    std::uint32_t inodeNumber;

    InodeType type;

    std::uint32_t size;

    std::uint32_t owner;

    std::uint32_t permissions;

    std::uint32_t linkCount;

    // Block number 0 (the superblock) is never a data block, so 0 means
    // "not allocated" in every pointer below.
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

    // Updates modifiedAt to the current wall-clock time.
    void touch();

    // Encodes into exactly DISK_RECORD_SIZE bytes (little-endian).
    void serialize(std::uint8_t* out) const;

    static Inode deserialize(const std::uint8_t* in);
};

#endif
