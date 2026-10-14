#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include "page_table.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

class MemoryManager {
public:
    static constexpr std::size_t PAGE_SIZE = 4096;
    static constexpr std::size_t DEFAULT_PHYSICAL_MEMORY =
        64 * 1024;

    explicit MemoryManager(
        std::size_t physicalMemorySize =
            DEFAULT_PHYSICAL_MEMORY
    );

    bool allocateProcess(
        std::uint32_t pid,
        std::size_t virtualMemorySize
    );

    bool freeProcess(
        std::uint32_t pid
    );

    bool write(
        std::uint32_t pid,
        std::size_t virtualAddress,
        std::uint8_t value
    );

    bool read(
        std::uint32_t pid,
        std::size_t virtualAddress,
        std::uint8_t& value
    ) const;

    std::size_t translateAddress(
        std::uint32_t pid,
        std::size_t virtualAddress
    ) const;

    void printMemoryMap() const;

    std::size_t getPhysicalMemorySize() const;

    std::size_t getFreeMemory() const;

private:
    std::vector<std::uint8_t> physicalMemory_;

    std::vector<bool> frameBitmap_;

    std::vector<PageTable> pageTables_;

    PageTable* findPageTable(
        std::uint32_t pid
    );

    const PageTable* findPageTable(
        std::uint32_t pid
    ) const;

    int allocateFrame();

    void freeFrame(
        std::size_t frameNumber
    );
};

#endif
