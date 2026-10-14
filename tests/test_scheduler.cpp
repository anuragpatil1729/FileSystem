
#include "scheduler.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    Scheduler scheduler(2);

    const auto pid1 = scheduler.createProcess("A", 5);
    const auto pid2 = scheduler.createProcess("B", 3);

    assert(pid1 == 1);
    assert(pid2 == 2);

    std::cout << "[PASS] Processes created\n";

    scheduler.run();

    const std::vector<std::uint32_t> expected = {
        1, 2, 1, 2, 1
    };

    assert(scheduler.getExecutionLog() == expected);
    assert(!scheduler.hasProcesses());

    std::cout << "[PASS] Round-Robin execution order\n";
    std::cout << "Execution: ";

    for (auto pid : scheduler.getExecutionLog()) {
        std::cout << "P" << pid << " ";
    }

    std::cout << "\nAll scheduler tests passed.\n";
    return 0;
}

