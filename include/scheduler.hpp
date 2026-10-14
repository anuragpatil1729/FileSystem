
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "process.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

struct GanttSlice {
    std::uint32_t pid;
    std::string name;
    std::size_t startTime;
    std::size_t endTime;
};

struct SchedulingRecord {
    std::uint32_t pid;
    std::string name;
    std::size_t burstTime;
    std::size_t completionTime;
    std::size_t waitingTime;
    std::size_t turnaroundTime;
};

class Scheduler {
public:
    explicit Scheduler(std::size_t timeQuantum = 2);

    std::uint32_t createProcess(
        const std::string& name,
        std::size_t burstTime,
        int priority = 0
    );

    void addProcess(
        const std::shared_ptr<Process>& process
    );

    void run();

    bool hasProcesses() const;

    std::size_t getTimeQuantum() const;

    const std::vector<std::uint32_t>&
    getExecutionLog() const;

    const std::vector<GanttSlice>&
    getTimeline() const;

    std::vector<SchedulingRecord>
    getSchedulingRecords() const;

    std::size_t getTotalExecutionTime() const;

private:
    std::size_t timeQuantum_;
    std::uint32_t nextPid_;
    std::size_t elapsedTime_;

    std::queue<std::shared_ptr<Process>> readyQueue_;

    std::vector<std::shared_ptr<Process>> processes_;

    std::vector<std::uint32_t> executionLog_;

    std::vector<GanttSlice> timeline_;

    std::unordered_map<std::uint32_t, std::size_t>
        completionTimes_;

    void scheduleProcess(
        const std::shared_ptr<Process>& process
    );
};

#endif

