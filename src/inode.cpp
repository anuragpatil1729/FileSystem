#include "inode.hpp"

#include <chrono>

void Inode::initialize(
    std::uint32_t number,
    InodeType inodeType
) {
    inodeNumber = number;

    type = inodeType;

    size = 0;

    owner = 0;

    permissions = 0644;

    linkCount = 1;

    indirectBlock = 0;

    for (std::size_t i = 0; i < DIRECT_BLOCKS; ++i) {
        directBlocks[i] = 0;
    }

    const auto now =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            std::chrono::system_clock::now()
            .time_since_epoch()
        ).count();

    createdAt =
        static_cast<std::uint64_t>(now);

    modifiedAt =
        static_cast<std::uint64_t>(now);
}

bool Inode::isFree() const {
    return type == InodeType::FREE;
}

bool Inode::isFile() const {
    return type == InodeType::FILE;
}

bool Inode::isDirectory() const {
    return type == InodeType::DIRECTORY;
}
