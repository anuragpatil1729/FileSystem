
#include "scheduler.hpp"

#include <algorithm>

Scheduler::Scheduler(std::size_t timeQuantum)
    : timeQuantum_(timeQuantum == 0 ? 1 : timeQuantum),
      nextPid_(1),
      elapsedTime_(0) {
}

std::uint32_t Scheduler::createProcess(
    const std::string& name,
    std::size_t burstTime,
    int priority
) {
    if (burstTime == 0) {
        return 0;
    }

    auto process = std::make_shared<Process>(
        nextPid_,
        name,
        burstTime,
        priority
    );

    ++nextPid_;
    addProcess(process);

    return process->getPid();
}

void Scheduler::addProcess(
    const std::shared_ptr<Process>& process
) {
    if (!process || process->isFinished()) {
        return;
    }

    process->setState(ProcessState::READY);

    processes_.push_back(process);
    readyQueue_.push(process);
}

void Scheduler::scheduleProcess(
    const std::shared_ptr<Process>& process
) {
    if (!process || process->isFinished()) {
        return;
    }

    const std::size_t startTime = elapsedTime_;

    const std::size_t executionTime = std::min(
        timeQuantum_,
        process->getRemainingTime()
    );

    process->execute(executionTime);

    elapsedTime_ += executionTime;

    executionLog_.push_back(process->getPid());

    timeline_.push_back({
        process->getPid(),
        process->getName(),
        startTime,
        elapsedTime_
    });

    if (process->isFinished()) {
        completionTimes_[process->getPid()] = elapsedTime_;
    } else {
        readyQueue_.push(process);
    }
}

void Scheduler::run() {
    executionLog_.clear();
    timeline_.clear();
    completionTimes_.clear();
    elapsedTime_ = 0;

    while (!readyQueue_.empty()) {
        auto process = readyQueue_.front();
        readyQueue_.pop();

        scheduleProcess(process);
    }
}

bool Scheduler::hasProcesses() const {
    return !readyQueue_.empty();
}

std::size_t Scheduler::getTimeQuantum() const {
    return timeQuantum_;
}

const std::vector<std::uint32_t>&
Scheduler::getExecutionLog() const {
    return executionLog_;
}

const std::vector<GanttSlice>&
Scheduler::getTimeline() const {
    return timeline_;
}

std::vector<SchedulingRecord>
Scheduler::getSchedulingRecords() const {
    std::vector<SchedulingRecord> records;

    for (const auto& process : processes_) {
        if (!process) {
            continue;
        }

        const auto found =
            completionTimes_.find(process->getPid());

        // Only report processes completed by the last run.
        if (found == completionTimes_.end()) {
            continue;
        }

        const std::size_t completionTime = found->second;
        const std::size_t turnaroundTime = completionTime;
        const std::size_t burstTime = process->getBurstTime();

        const std::size_t waitingTime =
            turnaroundTime >= burstTime
                ? turnaroundTime - burstTime
                : 0;

        records.push_back({
            process->getPid(),
            process->getName(),
            burstTime,
            completionTime,
            waitingTime,
            turnaroundTime
        });
    }

    return records;
}

std::size_t Scheduler::getTotalExecutionTime() const {
    return elapsedTime_;
}

