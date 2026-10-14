#ifndef BYTE_IO_HPP
#define BYTE_IO_HPP

#include <cstddef>
#include <cstdint>

/*
 * Explicit little-endian encoding helpers used for every on-disk
 * structure. Serialising field-by-field (instead of memcpy'ing structs)
 * keeps the disk format independent of compiler padding, alignment,
 * and host endianness.
 */
namespace byteio {

inline void putU32(std::uint8_t* out, std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i) {
        out[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu);
    }
}

inline void putU64(std::uint8_t* out, std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i) {
        out[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu);
    }
}

inline std::uint32_t getU32(const std::uint8_t* in) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(in[i]) << (8 * i);
    }
    return value;
}

inline std::uint64_t getU64(const std::uint8_t* in) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(in[i]) << (8 * i);
    }
    return value;
}

} // namespace byteio

#endif
