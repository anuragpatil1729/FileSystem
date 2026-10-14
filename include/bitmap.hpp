#ifndef BITMAP_HPP
#define BITMAP_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

class Bitmap {
public:
    Bitmap();

    explicit Bitmap(std::size_t bitCount);

    void initialize(std::size_t bitCount);

    bool set(std::size_t index);

    bool clear(std::size_t index);

    bool isSet(std::size_t index) const;

    bool isFree(std::size_t index) const;

    std::size_t findFree() const;

    std::size_t size() const;

    std::size_t countFree() const;

    std::size_t countUsed() const;

private:
    std::vector<std::uint8_t> bits_;
    std::size_t bitCount_;

    bool validIndex(std::size_t index) const;
};

#endif
