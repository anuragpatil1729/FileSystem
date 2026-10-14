
#include "filesystem.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include "memory_manager.hpp"

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

void printHelp() {
    std::cout
        << "\n===== VirtualFileSystem Commands =====\n"
        << "help                 Show commands\n"
        << "ls [path]            List directory\n"
        << "create <path>        Create a file\n"
        << "mkdir <path>         Create a directory\n"
        << "write <path> <text>  Write text to a file\n"
        << "read <path>          Read a file\n"
        << "rm <path>            Delete a file\n"
        << "rmdir <path>         Delete a directory\n"
        << "info                 Show filesystem information\n"
        << "cpu                  Run CPU scheduler demo\n"
        << "mem                  Run memory manager demo\n"
        << "exit                 Quit\n"
        << "======================================\n\n";
}

void runCpuDemo() {
    Scheduler scheduler(2);

    scheduler.createProcess("Process-A", 5, 1);
    scheduler.createProcess("Process-B", 3, 1);

    std::cout << "\n========== CPU SCHEDULER ==========\n";
    std::cout << "Algorithm: Round Robin\n";
    std::cout << "Time quantum: "
              << scheduler.getTimeQuantum() << "\n\n";

    scheduler.run();

    std::cout << "Gantt Timeline:\n";

    for (const auto& slice : scheduler.getTimeline()) {
        std::cout << "| P" << slice.pid
                  << " [" << slice.startTime
                  << "-" << slice.endTime << "] ";
    }

    std::cout << "|\n\n";

    std::cout << "Execution order: ";

    for (const auto pid : scheduler.getExecutionLog()) {
        std::cout << "P" << pid << " ";
    }

    std::cout << "\n\n";

    std::cout << "Process Statistics:\n";
    std::cout << std::left
              << std::setw(8) << "PID"
              << std::setw(10) << "Burst"
              << std::setw(14) << "Completion"
              << std::setw(10) << "Waiting"
              << std::setw(12) << "Turnaround"
              << '\n';

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;
    std::size_t count = 0;

    for (const auto& record : scheduler.getSchedulingRecords()) {
        std::cout << std::left
                  << std::setw(8) << ("P" + std::to_string(record.pid))
                  << std::setw(10) << record.burstTime
                  << std::setw(14) << record.completionTime
                  << std::setw(10) << record.waitingTime
                  << std::setw(12) << record.turnaroundTime
                  << '\n';

        totalWaiting += record.waitingTime;
        totalTurnaround += record.turnaroundTime;
        ++count;
    }

    if (count > 0) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\nAverage waiting time: "
                  << totalWaiting / count << '\n';

        std::cout << "Average turnaround time: "
                  << totalTurnaround / count << '\n';
    }

    std::cout << "Total CPU time: "
              << scheduler.getTotalExecutionTime() << '\n';

    std::cout << "===================================\n";
}

void runMemoryDemo() {
    MemoryManager memoryManager;

    constexpr int PID = 1;
    constexpr std::size_t PROCESS_SIZE = 8192;
    constexpr std::size_t VIRTUAL_ADDRESS = 100;

    std::cout << "\n========== MEMORY MANAGER ==========\n";

    if (!memoryManager.allocateProcess(PID, PROCESS_SIZE)) {
        std::cout << "Failed to allocate memory for process.\n";
        return;
    }

    std::cout << "Allocated " << PROCESS_SIZE
              << " bytes for process " << PID << ".\n";

    const std::size_t physicalAddress =
        memoryManager.translateAddress(PID, VIRTUAL_ADDRESS);

    if (physicalAddress == std::numeric_limits<std::size_t>::max()) {
        std::cout << "Address translation failed.\n";
    } else {
        std::cout << "Virtual address: " << VIRTUAL_ADDRESS << '\n';
        std::cout << "Physical address: " << physicalAddress << '\n';
    }

    if (memoryManager.freeProcess(PID)) {
        std::cout << "Process memory released successfully.\n";
    }

    std::cout << "====================================\n";
}

} // namespace

int main() {
    std::cout << "Initializing VirtualFileSystem...\n";

    FileSystem fs("data/virtual_disk.bin");

    // Mount existing metadata first. Format only if mounting fails.
    if (fs.mount()) {
        std::cout << "Existing filesystem mounted successfully!\n";
    } else {
        std::cout << "No valid filesystem found. Formatting disk...\n";

        if (!fs.format()) {
            std::cerr << "ERROR: Failed to format the virtual disk.\n";
            return 1;
        }

        std::cout << "New filesystem formatted successfully!\n";
    }

    std::cout << "Filesystem ready!\n";
    printHelp();

    std::string line;

    while (true) {
        std::cout << "vfs> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command == "help") {
            printHelp();
        }
        else if (command == "ls") {
            std::string path = "/";
            input >> path;

            const auto entries = fs.listDirectory(path);

            if (entries.empty()) {
                std::cout << "Directory is empty.\n";
            } else {
                for (const auto& entry : entries) {
                    std::cout << entry << '\n';
                }
            }
        }
        else if (command == "create") {
            std::string path;
            input >> path;

            if (path.empty()) {
                std::cout << "Usage: create <path>\n";
            } else if (fs.createFile(path)) {
                std::cout << "File created: " << path << '\n';
            } else {
                std::cout << "Failed to create file.\n";
            }
        }
        else if (command == "mkdir") {
            std::string path;
            input >> path;

            if (path.empty()) {
                std::cout << "Usage: mkdir <path>\n";
            } else if (fs.createDirectory(path)) {
                std::cout << "Directory created: " << path << '\n';
            } else {
                std::cout << "Failed to create directory.\n";
            }
        }
        else if (command == "write") {
            std::string path;
            input >> path;

            std::string data;
            std::getline(input >> std::ws, data);

            if (path.empty() || data.empty()) {
                std::cout << "Usage: write <path> <text>\n";
            } else if (fs.writeFile(path, data)) {
                std::cout << "File written successfully.\n";
            } else {
                std::cout << "Failed to write file.\n";
            }
        }
        else if (command == "read") {
            std::string path;
            input >> path;

            if (path.empty()) {
                std::cout << "Usage: read <path>\n";
            } else if (!fs.exists(path)) {
                std::cout << "File not found.\n";
            } else {
                const std::string data = fs.readFile(path);
                std::cout << data << '\n';
            }
        }
        else if (command == "rm") {
            std::string path;
            input >> path;

            if (path.empty()) {
                std::cout << "Usage: rm <path>\n";
            } else if (fs.deleteFile(path)) {
                std::cout << "File deleted: " << path << '\n';
            } else {
                std::cout << "Failed to delete file.\n";
            }
        }
        else if (command == "rmdir") {
            std::string path;
            input >> path;

            if (path.empty()) {
                std::cout << "Usage: rmdir <path>\n";
            } else if (fs.deleteDirectory(path)) {
                std::cout << "Directory deleted: " << path << '\n';
            } else {
                std::cout << "Failed to delete directory.\n";
            }
        }
        else if (command == "info") {
            fs.printInfo();
        }
        else if (command == "cpu") {
            runCpuDemo();
        }
        else if (command == "mem") {
            runMemoryDemo();
        }
        else if (command == "exit") {
            std::cout << "Unmounting filesystem...\n";
            fs.unmount();
            std::cout << "Goodbye!\n";
            return 0;
        }
        else {
            std::cout << "Unknown command. Type 'help' for commands.\n";
        }

        std::cout << '\n';
    }

    fs.unmount();
    return 0;
}

