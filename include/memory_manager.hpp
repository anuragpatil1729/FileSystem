#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include "page_table.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class AllocationPolicy {
    EAGER,
    DEMAND_PAGING
};

enum class ReplacementPolicy {
    FIFO,
    LRU
};

struct FrameMetadata {
    std::uint32_t pid;
    std::size_t virtualPage;
    std::size_t allocationTime;
    std::size_t lastAccessTime;
};

class MemoryManager {
public:
    static constexpr std::size_t PAGE_SIZE = 4096;
    static constexpr std::size_t DEFAULT_PHYSICAL_MEMORY = 64 * 1024;

    explicit MemoryManager(
        std::size_t physicalMemorySize = DEFAULT_PHYSICAL_MEMORY
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
    );

    // Const overload for read-only non-faulting translation
    std::size_t translateAddress(
        std::uint32_t pid,
        std::size_t virtualAddress
    ) const;

    // Mutating translation that can trigger demand paging if enabled
    std::size_t translateOrFault(
        std::uint32_t pid,
        std::size_t virtualAddress,
        bool isWrite = false
    );

    void printMemoryMap() const;

    std::size_t getPhysicalMemorySize() const;

    std::size_t getFreeMemory() const;

    // Demand paging and page replacement configuration & metrics
    void setAllocationPolicy(AllocationPolicy policy);
    AllocationPolicy getAllocationPolicy() const;

    void setReplacementPolicy(ReplacementPolicy policy);
    ReplacementPolicy getReplacementPolicy() const;

    std::size_t getPageFaultCount() const;
    std::size_t getPageReplacementCount() const;
    void resetStatistics();

    bool handlePageFault(std::uint32_t pid, std::size_t virtualPage);

private:
    std::vector<std::uint8_t> physicalMemory_;
    std::vector<bool> frameBitmap_;
    std::vector<PageTable> pageTables_;
    std::vector<FrameMetadata> frameMetadata_;

    AllocationPolicy allocationPolicy_;
    ReplacementPolicy replacementPolicy_;

    std::size_t timerCounter_;
    std::size_t pageFaultCount_;
    std::size_t pageReplacementCount_;

    PageTable* findPageTable(std::uint32_t pid);
    const PageTable* findPageTable(std::uint32_t pid) const;

    int allocateFrame();
    void freeFrame(std::size_t frameNumber);
    int selectVictimFrame() const;
};

#endif
