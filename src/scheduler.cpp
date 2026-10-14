#include "scheduler.hpp"

#include <algorithm>
#include <limits>

Scheduler::Scheduler(std::size_t timeQuantum)
    : timeQuantum_(timeQuantum == 0 ? 1 : timeQuantum),
      nextPid_(1),
      elapsedTime_(0),
      totalBusyTime_(0) {
}

void Scheduler::setTimeQuantum(std::size_t quantum) {
    timeQuantum_ = quantum == 0 ? 1 : quantum;
}

std::uint32_t Scheduler::createProcess(
    const std::string& name,
    std::size_t burstTime,
    int priority,
    std::size_t arrivalTime
) {
    if (burstTime == 0) {
        return 0;
    }

    auto process = std::make_shared<Process>(
        nextPid_,
        name,
        burstTime,
        priority,
        arrivalTime
    );

    ++nextPid_;
    addProcess(process);
    return process->getPid();
}

void Scheduler::addProcess(
    const std::shared_ptr<Process>& process
) {
    if (!process || process->getBurstTime() == 0) {
        return;
    }

    for (const auto& p : processes_) {
        if (p == process || (p && p->getPid() == process->getPid())) {
            return;
        }
    }

    process->setState(ProcessState::READY);
    processes_.push_back(process);
}

void Scheduler::run() {
    executionLog_.clear();
    timeline_.clear();
    completionTimes_.clear();
    responseTimes_.clear();
    enqueuedPids_.clear();
    while (!readyQueue_.empty()) {
        readyQueue_.pop();
    }

    elapsedTime_ = 0;
    totalBusyTime_ = 0;

    for (auto& p : processes_) {
        if (p) {
            p->reset();
        }
    }

    auto enqueueNewlyArrived = [&](const std::shared_ptr<Process>& runningProcess) {
        for (const auto& p : processes_) {
            if (p && p != runningProcess && !p->isFinished() && p->getArrivalTime() <= elapsedTime_) {
                if (enqueuedPids_.find(p->getPid()) == enqueuedPids_.end()) {
                    readyQueue_.push(p);
                    enqueuedPids_.insert(p->getPid());
                }
            }
        }
    };

    enqueueNewlyArrived(nullptr);

    while (true) {
        if (readyQueue_.empty()) {
            std::size_t nextArrival = std::numeric_limits<std::size_t>::max();
            for (const auto& p : processes_) {
                if (p && !p->isFinished()) {
                    if (p->getArrivalTime() < nextArrival) {
                        nextArrival = p->getArrivalTime();
                    }
                }
            }

            if (nextArrival == std::numeric_limits<std::size_t>::max()) {
                break;
            }

            if (nextArrival > elapsedTime_) {
                elapsedTime_ = nextArrival;
            }
            enqueueNewlyArrived(nullptr);
            if (readyQueue_.empty()) {
                break;
            }
        }

        auto process = readyQueue_.front();
        readyQueue_.pop();
        enqueuedPids_.erase(process->getPid());

        if (process->isFinished()) {
            continue;
        }

        if (responseTimes_.find(process->getPid()) == responseTimes_.end()) {
            const std::size_t resp = elapsedTime_ >= process->getArrivalTime()
                                         ? elapsedTime_ - process->getArrivalTime()
                                         : 0;
            responseTimes_[process->getPid()] = resp;
        }

        const std::size_t startTime = elapsedTime_;
        const std::size_t executionTime = std::min(
            timeQuantum_,
            process->getRemainingTime()
        );

        process->execute(executionTime);
        elapsedTime_ += executionTime;
        totalBusyTime_ += executionTime;

        executionLog_.push_back(process->getPid());
        timeline_.push_back({
            process->getPid(),
            process->getName(),
            startTime,
            elapsedTime_
        });

        // Enqueue newly arrived processes during this time slice before re-adding current
        enqueueNewlyArrived(process);

        if (process->isFinished()) {
            completionTimes_[process->getPid()] = elapsedTime_;
        } else {
            readyQueue_.push(process);
            enqueuedPids_.insert(process->getPid());
        }
    }
}

bool Scheduler::hasProcesses() const {
    for (const auto& p : processes_) {
        if (p && !p->isFinished()) {
            return true;
        }
    }
    return false;
}

std::size_t Scheduler::getTimeQuantum() const {
    return timeQuantum_;
}

const std::vector<std::uint32_t>& Scheduler::getExecutionLog() const {
    return executionLog_;
}

const std::vector<GanttSlice>& Scheduler::getTimeline() const {
    return timeline_;
}

std::vector<SchedulingRecord> Scheduler::getSchedulingRecords() const {
    std::vector<SchedulingRecord> records;

    for (const auto& process : processes_) {
        if (!process) continue;

        const auto found = completionTimes_.find(process->getPid());
        if (found == completionTimes_.end()) {
            continue;
        }

        const std::size_t completionTime = found->second;
        const std::size_t arrivalTime = process->getArrivalTime();
        const std::size_t turnaroundTime = completionTime >= arrivalTime
                                               ? completionTime - arrivalTime
                                               : 0;
        const std::size_t burstTime = process->getBurstTime();
        const std::size_t waitingTime = turnaroundTime >= burstTime
                                            ? turnaroundTime - burstTime
                                            : 0;

        std::size_t resp = 0;
        const auto rFound = responseTimes_.find(process->getPid());
        if (rFound != responseTimes_.end()) {
            resp = rFound->second;
        }

        records.push_back({
            process->getPid(),
            process->getName(),
            burstTime,
            arrivalTime,
            completionTime,
            waitingTime,
            turnaroundTime,
            resp
        });
    }

    return records;
}

std::size_t Scheduler::getTotalExecutionTime() const {
    return elapsedTime_;
}

std::size_t Scheduler::getTotalBusyTime() const {
    return totalBusyTime_;
}

double Scheduler::getAverageWaitingTime() const {
    const auto records = getSchedulingRecords();
    if (records.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& r : records) {
        sum += static_cast<double>(r.waitingTime);
    }
    return sum / static_cast<double>(records.size());
}

double Scheduler::getAverageTurnaroundTime() const {
    const auto records = getSchedulingRecords();
    if (records.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& r : records) {
        sum += static_cast<double>(r.turnaroundTime);
    }
    return sum / static_cast<double>(records.size());
}

double Scheduler::getCpuUtilization() const {
    if (elapsedTime_ == 0) return 0.0;
    return (static_cast<double>(totalBusyTime_) / static_cast<double>(elapsedTime_)) * 100.0;
}

void Scheduler::reset() {
    elapsedTime_ = 0;
    totalBusyTime_ = 0;
    while (!readyQueue_.empty()) readyQueue_.pop();
    executionLog_.clear();
    timeline_.clear();
    completionTimes_.clear();
    responseTimes_.clear();
    enqueuedPids_.clear();
    for (auto& p : processes_) {
        if (p) p->reset();
    }
}

void Scheduler::clear() {
    reset();
    processes_.clear();
    nextPid_ = 1;
}
