#include "memory_manager.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#define TEST_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main() {
    // 1. Baseline eager allocation, translation, read/write (preserves original test)
    {
        MemoryManager memory(16 * 1024);

        const std::size_t initialFree = memory.getFreeMemory();

        TEST_CHECK(memory.allocateProcess(1, 8192));
        std::cout << "[PASS] Process memory allocated\n";

        const std::size_t physicalAddress =
            memory.translateAddress(1, 4200);

        TEST_CHECK(physicalAddress != static_cast<std::size_t>(-1));
        TEST_CHECK(physicalAddress < memory.getPhysicalMemorySize());
        std::cout << "[PASS] Virtual address translated\n";

        const std::uint8_t expectedValue = 99;
        std::uint8_t actualValue = 0;

        TEST_CHECK(memory.write(1, 4200, expectedValue));
        TEST_CHECK(memory.read(1, 4200, actualValue));
        TEST_CHECK(actualValue == expectedValue);
        std::cout << "[PASS] Memory read/write\n";

        // Duplicate process allocation rejected
        TEST_CHECK(!memory.allocateProcess(1, 4096));
        // Nonexistent process rejected
        TEST_CHECK(!memory.read(999, 0, actualValue));
        // Virtual address bounds check: 8192 allocated -> address 8200 is out of bounds
        TEST_CHECK(!memory.write(1, 8200, 50));
        TEST_CHECK(!memory.read(1, 8200, actualValue));
        std::cout << "[PASS] Invalid operations and bounds rejected\n";

        TEST_CHECK(memory.freeProcess(1));
        TEST_CHECK(memory.getFreeMemory() == initialFree);
        std::cout << "[PASS] Process memory released\n";
    }

    // 2. Demand Paging with FIFO Page Replacement
    {
        // 2 frames (8192 bytes physical)
        MemoryManager memory(8192);
        memory.setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
        memory.setReplacementPolicy(ReplacementPolicy::FIFO);

        // Allocate 3 virtual pages (12288 bytes) to PID 1
        TEST_CHECK(memory.allocateProcess(1, 12288));
        // Frames are not allocated eagerly
        TEST_CHECK(memory.getFreeMemory() == 8192);
        TEST_CHECK(memory.getPageFaultCount() == 0);

        // Access page 0 (offset 100) -> triggers 1st page fault
        TEST_CHECK(memory.write(1, 100, 42));
        TEST_CHECK(memory.getPageFaultCount() == 1);
        TEST_CHECK(memory.getPageReplacementCount() == 0);
        TEST_CHECK(memory.getFreeMemory() == 4096);

        // Access page 1 (offset 4096 + 100) -> triggers 2nd page fault
        TEST_CHECK(memory.write(1, 4196, 43));
        TEST_CHECK(memory.getPageFaultCount() == 2);
        TEST_CHECK(memory.getPageReplacementCount() == 0);
        TEST_CHECK(memory.getFreeMemory() == 0);

        // Access page 2 (offset 8192 + 100) -> triggers 3rd page fault AND 1 page replacement (FIFO evicts page 0)
        TEST_CHECK(memory.write(1, 8292, 44));
        TEST_CHECK(memory.getPageFaultCount() == 3);
        TEST_CHECK(memory.getPageReplacementCount() == 1);

        // Page 0 was evicted: non-faulting translateAddress should report unmapped (-1)
        TEST_CHECK(memory.translateAddress(1, 100) == static_cast<std::size_t>(-1));
        // Page 1 and Page 2 are currently mapped
        TEST_CHECK(memory.translateAddress(1, 4196) != static_cast<std::size_t>(-1));
        TEST_CHECK(memory.translateAddress(1, 8292) != static_cast<std::size_t>(-1));

        std::cout << "[PASS] Demand paging and FIFO page replacement\n";
    }

    // 3. LRU Page Replacement
    {
        // 2 frames
        MemoryManager memory(8192);
        memory.setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
        memory.setReplacementPolicy(ReplacementPolicy::LRU);

        TEST_CHECK(memory.allocateProcess(1, 12288));

        // Load page 0 and page 1
        TEST_CHECK(memory.write(1, 10, 1));
        TEST_CHECK(memory.write(1, 4096 + 10, 2));

        // Read page 0 again -> makes page 0 most recently used, page 1 is now LRU!
        std::uint8_t val = 0;
        TEST_CHECK(memory.read(1, 10, val));
        TEST_CHECK(val == 1);

        // Access page 2 -> LRU must evict page 1, keeping page 0 in memory!
        TEST_CHECK(memory.write(1, 8192 + 10, 3));
        TEST_CHECK(memory.getPageReplacementCount() == 1);

        // Page 0 should still be in memory!
        TEST_CHECK(memory.translateAddress(1, 10) != static_cast<std::size_t>(-1));
        // Page 1 was evicted!
        TEST_CHECK(memory.translateAddress(1, 4096 + 10) == static_cast<std::size_t>(-1));

        std::cout << "[PASS] LRU page replacement algorithm\n";
    }

    std::cout << "\nAll memory tests passed.\n";
    return 0;
}
