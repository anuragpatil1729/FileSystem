#include "inode.hpp"

#include "byte_io.hpp"

#include <chrono>
#include <cstring>

namespace {
std::uint64_t nowSeconds() {
    const auto now =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            std::chrono::system_clock::now()
            .time_since_epoch()
        ).count();
    return now < 0 ? 0 : static_cast<std::uint64_t>(now);
}
} // namespace

void Inode::initialize(
    std::uint32_t number,
    InodeType inodeType
) {
    inodeNumber = number;

    type = inodeType;

    size = 0;

    owner = 0;

    permissions = inodeType == InodeType::DIRECTORY ? 0755 : 0644;

    linkCount = 1;

    indirectBlock = 0;

    for (std::size_t i = 0; i < DIRECT_BLOCKS; ++i) {
        directBlocks[i] = 0;
    }

    const std::uint64_t now = nowSeconds();

    createdAt = now;

    modifiedAt = now;
}

void Inode::touch() {
    modifiedAt = nowSeconds();
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

/*
 * Record layout (offsets in bytes):
 *   0 inodeNumber   4 type   8 size   12 owner   16 permissions
 *  20 linkCount    24 directBlocks[12]   72 indirectBlock
 *  76 createdAt    84 modifiedAt         92..127 reserved (zero)
 */
void Inode::serialize(std::uint8_t* out) const {
    std::memset(out, 0, DISK_RECORD_SIZE);
    byteio::putU32(out + 0, inodeNumber);
    out[4] = static_cast<std::uint8_t>(type);
    byteio::putU32(out + 8, size);
    byteio::putU32(out + 12, owner);
    byteio::putU32(out + 16, permissions);
    byteio::putU32(out + 20, linkCount);
    for (std::size_t i = 0; i < DIRECT_BLOCKS; ++i) {
        byteio::putU32(out + 24 + 4 * i, directBlocks[i]);
    }
    byteio::putU32(out + 72, indirectBlock);
    byteio::putU64(out + 76, createdAt);
    byteio::putU64(out + 84, modifiedAt);
}

Inode Inode::deserialize(const std::uint8_t* in) {
    Inode inode{};
    inode.inodeNumber = byteio::getU32(in + 0);
    // Raw value is kept; FileSystem validation rejects unknown types.
    inode.type = static_cast<InodeType>(in[4]);
    inode.size = byteio::getU32(in + 8);
    inode.owner = byteio::getU32(in + 12);
    inode.permissions = byteio::getU32(in + 16);
    inode.linkCount = byteio::getU32(in + 20);
    for (std::size_t i = 0; i < DIRECT_BLOCKS; ++i) {
        inode.directBlocks[i] = byteio::getU32(in + 24 + 4 * i);
    }
    inode.indirectBlock = byteio::getU32(in + 72);
    inode.createdAt = byteio::getU64(in + 76);
    inode.modifiedAt = byteio::getU64(in + 84);
    return inode;
}
