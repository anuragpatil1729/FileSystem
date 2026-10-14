#include "bitmap.hpp"

#include <algorithm>
#include <cstring>

Bitmap::Bitmap()
    : bitCount_(0) {
}

Bitmap::Bitmap(std::size_t bitCount)
    : bitCount_(0) {
    initialize(bitCount);
}

void Bitmap::initialize(std::size_t bitCount) {
    bitCount_ = bitCount;

    /*
     * One byte stores 8 bits.
     */
    const std::size_t byteCount =
        (bitCount + 7) / 8;

    bits_.assign(byteCount, 0);
}

bool Bitmap::validIndex(std::size_t index) const {
    return index < bitCount_;
}

bool Bitmap::set(std::size_t index) {
    if (!validIndex(index)) {
        return false;
    }

    const std::size_t byteIndex = index / 8;
    const std::size_t bitIndex = index % 8;

    bits_[byteIndex] |=
        static_cast<std::uint8_t>(
            1u << bitIndex
        );

    return true;
}

bool Bitmap::clear(std::size_t index) {
    if (!validIndex(index)) {
        return false;
    }

    const std::size_t byteIndex = index / 8;
    const std::size_t bitIndex = index % 8;

    bits_[byteIndex] &=
        static_cast<std::uint8_t>(
            ~(1u << bitIndex)
        );

    return true;
}

bool Bitmap::isSet(std::size_t index) const {
    if (!validIndex(index)) {
        return false;
    }

    const std::size_t byteIndex = index / 8;
    const std::size_t bitIndex = index % 8;

    return (
        bits_[byteIndex] &
        static_cast<std::uint8_t>(
            1u << bitIndex
        )
    ) != 0;
}

bool Bitmap::isFree(std::size_t index) const {
    return validIndex(index) && !isSet(index);
}

std::size_t Bitmap::findFree() const {
    for (std::size_t i = 0; i < bitCount_; ++i) {
        if (!isSet(i)) {
            return i;
        }
    }

    return bitCount_;
}

std::size_t Bitmap::size() const {
    return bitCount_;
}

std::size_t Bitmap::countFree() const {
    std::size_t count = 0;

    for (std::size_t i = 0; i < bitCount_; ++i) {
        if (!isSet(i)) {
            ++count;
        }
    }

    return count;
}

std::size_t Bitmap::countUsed() const {
    return bitCount_ - countFree();
}

const std::vector<std::uint8_t>& Bitmap::getRawBits() const {
    return bits_;
}

const std::uint8_t* Bitmap::data() const {
    return bits_.data();
}

std::size_t Bitmap::byteSize() const {
    return bits_.size();
}

void Bitmap::loadBits(const std::uint8_t* buffer, std::size_t byteCount) {
    const std::size_t toCopy = std::min(byteCount, bits_.size());
    if (toCopy > 0 && buffer != nullptr) {
        std::memcpy(bits_.data(), buffer, toCopy);
    }
    // Zero out any leftover bits beyond bitCount_ in the last byte
    if (bitCount_ % 8 != 0 && !bits_.empty()) {
        const std::uint8_t mask = static_cast<std::uint8_t>((1u << (bitCount_ % 8)) - 1u);
        bits_.back() &= mask;
    }
}
