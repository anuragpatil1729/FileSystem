#include "scheduler.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#define TEST_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main() {
    // 1. Baseline Round-Robin execution (preserves original test)
    {
        Scheduler scheduler(2);

        const auto pid1 = scheduler.createProcess("A", 5);
        const auto pid2 = scheduler.createProcess("B", 3);

        TEST_CHECK(pid1 == 1);
        TEST_CHECK(pid2 == 2);
        std::cout << "[PASS] Processes created\n";

        scheduler.run();

        const std::vector<std::uint32_t> expected = {
            1, 2, 1, 2, 1
        };

        TEST_CHECK(scheduler.getExecutionLog() == expected);
        TEST_CHECK(!scheduler.hasProcesses());
        std::cout << "[PASS] Round-Robin execution order\n";

        // Verify metrics
        const auto records = scheduler.getSchedulingRecords();
        TEST_CHECK(records.size() == 2);
        // Process A: burst 5, completion 8, turnaround 8, waiting 3
        // Process B: burst 3, completion 7, turnaround 7, waiting 4
        TEST_CHECK(records[0].completionTime == 8);
        TEST_CHECK(records[0].waitingTime == 3);
        TEST_CHECK(records[1].completionTime == 7);
        TEST_CHECK(records[1].waitingTime == 4);

        TEST_CHECK(std::abs(scheduler.getAverageWaitingTime() - 3.5) < 1e-4);
        TEST_CHECK(std::abs(scheduler.getAverageTurnaroundTime() - 7.5) < 1e-4);
        TEST_CHECK(std::abs(scheduler.getCpuUtilization() - 100.0) < 1e-4);
        std::cout << "[PASS] Scheduling metrics calculated correctly\n";

        // Repeatable execution
        scheduler.run();
        TEST_CHECK(scheduler.getExecutionLog() == expected);
        std::cout << "[PASS] Repeatable execution verified\n";
    }

    // 2. Arrival Times and Idle CPU handling
    {
        Scheduler scheduler(2);
        // P1 arrives at t=0, burst=2
        // P2 arrives at t=5 (gap between 2 and 5), burst=2
        scheduler.createProcess("Early", 2, 0, 0);
        scheduler.createProcess("Late", 2, 0, 5);

        scheduler.run();
        const std::vector<std::uint32_t> expected = {1, 2};
        TEST_CHECK(scheduler.getExecutionLog() == expected);

        const auto records = scheduler.getSchedulingRecords();
        TEST_CHECK(records[0].completionTime == 2);
        TEST_CHECK(records[1].completionTime == 7);
        TEST_CHECK(records[1].waitingTime == 0); // arrived at 5, started at 5
        TEST_CHECK(scheduler.getTotalExecutionTime() == 7);
        TEST_CHECK(scheduler.getTotalBusyTime() == 4);
        std::cout << "[PASS] Process arrival times and idle handling\n";
    }

    // 3. Validation: zero burst rejected, duplicate processes rejected
    {
        Scheduler scheduler(1);
        TEST_CHECK(scheduler.createProcess("Bad", 0) == 0);

        auto p = std::make_shared<Process>(10, "Unique", 4);
        scheduler.addProcess(p);
        scheduler.addProcess(p); // Duplicate add
        TEST_CHECK(scheduler.createProcess("DupPid", 3) != 10);
        std::cout << "[PASS] Invalid and duplicate process validation\n";
    }

    std::cout << "\nAll scheduler tests passed.\n";
    return 0;
}
