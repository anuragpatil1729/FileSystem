#include "process.hpp"

#include <algorithm>

Process::Process(
    std::uint32_t pid,
    const std::string& name,
    std::size_t burstTime,
    int priority,
    std::size_t arrivalTime
)
    : pid_(pid),
      name_(name),
      burstTime_(burstTime),
      remainingTime_(burstTime),
      priority_(priority),
      arrivalTime_(arrivalTime),
      state_(ProcessState::NEW) {
}

std::uint32_t Process::getPid() const {
    return pid_;
}

const std::string& Process::getName() const {
    return name_;
}

std::size_t Process::getBurstTime() const {
    return burstTime_;
}

std::size_t Process::getRemainingTime() const {
    return remainingTime_;
}

int Process::getPriority() const {
    return priority_;
}

std::size_t Process::getArrivalTime() const {
    return arrivalTime_;
}

ProcessState Process::getState() const {
    return state_;
}

void Process::setState(
    ProcessState state
) {
    state_ = state;
}

void Process::execute(
    std::size_t timeSlice
) {
    if (
        state_ == ProcessState::TERMINATED ||
        remainingTime_ == 0
    ) {
        return;
    }

    state_ = ProcessState::RUNNING;

    std::size_t executionTime =
        std::min(
            timeSlice,
            remainingTime_
        );

    remainingTime_ -= executionTime;

    if (remainingTime_ == 0) {
        state_ = ProcessState::TERMINATED;
    } else {
        state_ = ProcessState::READY;
    }
}

bool Process::isFinished() const {
    return remainingTime_ == 0;
}

void Process::reset() {
    remainingTime_ = burstTime_;
    state_ = ProcessState::READY;
}
