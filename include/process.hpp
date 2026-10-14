#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

class Process {
public:
    Process(
        std::uint32_t pid,
        const std::string& name,
        std::size_t burstTime,
        int priority = 0
    );

    std::uint32_t getPid() const;

    const std::string& getName() const;

    std::size_t getBurstTime() const;

    std::size_t getRemainingTime() const;

    int getPriority() const;

    ProcessState getState() const;

    void setState(
        ProcessState state
    );

    void execute(
        std::size_t timeSlice
    );

    bool isFinished() const;

private:
    std::uint32_t pid_;

    std::string name_;

    std::size_t burstTime_;

    std::size_t remainingTime_;

    int priority_;

    ProcessState state_;
};

#endif
