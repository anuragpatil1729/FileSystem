
#include "memory_manager.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    MemoryManager memory(16 * 1024);

    const std::size_t initialFree = memory.getFreeMemory();

    assert(memory.allocateProcess(1, 8192));
    std::cout << "[PASS] Process memory allocated\n";

    const std::size_t physicalAddress =
        memory.translateAddress(1, 4200);

    assert(physicalAddress != static_cast<std::size_t>(-1));
    assert(physicalAddress < memory.getPhysicalMemorySize());

    std::cout << "[PASS] Virtual address translated\n";

    const std::uint8_t expectedValue = 99;
    std::uint8_t actualValue = 0;

    assert(memory.write(1, 4200, expectedValue));
    assert(memory.read(1, 4200, actualValue));
    assert(actualValue == expectedValue);

    std::cout << "[PASS] Memory read/write\n";

    assert(!memory.allocateProcess(1, 4096));
    assert(!memory.read(999, 0, actualValue));

    std::cout << "[PASS] Invalid operations rejected\n";

    assert(memory.freeProcess(1));
    assert(memory.getFreeMemory() == initialFree);

    std::cout << "[PASS] Process memory released\n";
    std::cout << "All memory tests passed.\n";

    return 0;
}

