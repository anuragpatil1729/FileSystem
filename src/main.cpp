#include "filesystem.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include "memory_manager.hpp"

#include <fstream>
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
        << "Filesystem:\n"
        << "  help                        Show available commands\n"
        << "  ls [path]                   List directory contents\n"
        << "  create <path>               Create an empty file\n"
        << "  mkdir <path>                Create a directory\n"
        << "  write <path> <text>         Write text to a file\n"
        << "  read <path>                 Read content from a file\n"
        << "  rm <path>                   Delete a regular file\n"
        << "  rmdir <path>                Delete an empty directory\n"
        << "  info                        Show filesystem geometry and status\n"
        << "  format [blocks] [inodes]    Format the virtual disk\n"
        << "\nCPU Scheduler:\n"
        << "  cpu                         Run CPU scheduler demo with predefined workload\n"
        << "  sched-add <name> <burst> [priority] [arrival]   Add process to scheduler\n"
        << "  sched-run [quantum]         Execute scheduler with configured/added processes\n"
        << "  sched-clear                 Clear all scheduled processes\n"
        << "\nVirtual Memory Manager:\n"
        << "  mem                         Run virtual memory manager demo\n"
        << "  mem-alloc <pid> <bytes>     Allocate virtual memory for a process\n"
        << "  mem-free <pid>              Release virtual memory of a process\n"
        << "  mem-write <pid> <addr> <val> Write byte to process virtual address\n"
        << "  mem-read <pid> <addr>       Read byte from process virtual address\n"
        << "  mem-map                     Display physical memory frame allocation map\n"
        << "  mem-policy <eager|demand> [fifo|lru] Configure memory policies\n"
        << "\nGeneral:\n"
        << "  exit                        Unmount and quit\n"
        << "======================================\n\n";
}

void printSchedulerResults(const Scheduler& scheduler) {
    std::cout << "\n========== CPU SCHEDULER ==========\n";
    std::cout << "Algorithm: Round Robin (Simulated CPU Time)\n";
    std::cout << "Time quantum: " << scheduler.getTimeQuantum() << " simulated time units\n\n";

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

    std::cout << "Process Statistics (Simulated CPU Clocks):\n";
    std::cout << std::left
              << std::setw(8) << "PID"
              << std::setw(12) << "Name"
              << std::setw(8) << "Burst"
              << std::setw(10) << "Arrival"
              << std::setw(14) << "Completion"
              << std::setw(10) << "Waiting"
              << std::setw(12) << "Turnaround"
              << std::setw(10) << "Response"
              << '\n';

    for (const auto& record : scheduler.getSchedulingRecords()) {
        std::cout << std::left
                  << std::setw(8) << ("P" + std::to_string(record.pid))
                  << std::setw(12) << record.name
                  << std::setw(8) << record.burstTime
                  << std::setw(10) << record.arrivalTime
                  << std::setw(14) << record.completionTime
                  << std::setw(10) << record.waitingTime
                  << std::setw(12) << record.turnaroundTime
                  << std::setw(10) << record.responseTime
                  << '\n';
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nAverage waiting time: " << scheduler.getAverageWaitingTime() << '\n';
    std::cout << "Average turnaround time: " << scheduler.getAverageTurnaroundTime() << '\n';
    std::cout << "CPU Utilization: " << scheduler.getCpuUtilization() << "%\n";
    std::cout << "Total simulated CPU time: " << scheduler.getTotalExecutionTime() << " units\n";
    std::cout << "===================================\n";
}

void runCpuDemo() {
    Scheduler scheduler(2);
    scheduler.createProcess("Process-A", 5, 1, 0);
    scheduler.createProcess("Process-B", 3, 1, 0);
    scheduler.run();
    printSchedulerResults(scheduler);
}

void runMemoryDemo() {
    MemoryManager memoryManager(16 * 1024);
    memoryManager.setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
    memoryManager.setReplacementPolicy(ReplacementPolicy::LRU);

    constexpr std::uint32_t PID = 1;
    constexpr std::size_t PROCESS_SIZE = 12 * 1024; // 3 virtual pages (12 KB)
    constexpr std::size_t VIRTUAL_ADDRESS = 4200;   // In page 1

    std::cout << "\n========== MEMORY MANAGER ==========\n";
    std::cout << "Physical memory: " << memoryManager.getPhysicalMemorySize() << " bytes\n";
    std::cout << "Policy: Demand Paging with LRU replacement\n";

    if (!memoryManager.allocateProcess(PID, PROCESS_SIZE)) {
        std::cout << "Failed to allocate memory for process.\n";
        return;
    }

    std::cout << "Allocated " << PROCESS_SIZE << " virtual bytes (3 pages) for process " << PID << ".\n";
    std::cout << "Free physical frames before access: " << (memoryManager.getFreeMemory() / MemoryManager::PAGE_SIZE) << "\n";

    // Write to virtual address 4200
    if (memoryManager.write(PID, VIRTUAL_ADDRESS, 0xAB)) {
        std::cout << "Wrote byte 0xAB to virtual address " << VIRTUAL_ADDRESS << " (triggered page fault)\n";
    }

    std::uint8_t readVal = 0;
    if (memoryManager.read(PID, VIRTUAL_ADDRESS, readVal)) {
        std::cout << "Read byte 0x" << std::hex << static_cast<int>(readVal) << std::dec
                  << " from virtual address " << VIRTUAL_ADDRESS << '\n';
    }

    const std::size_t physicalAddress = memoryManager.translateAddress(PID, VIRTUAL_ADDRESS);
    std::cout << "Virtual address " << VIRTUAL_ADDRESS << " -> Physical address " << physicalAddress << '\n';
    std::cout << "Page faults: " << memoryManager.getPageFaultCount() << ", Page replacements: "
              << memoryManager.getPageReplacementCount() << '\n';

    memoryManager.printMemoryMap();

    if (memoryManager.freeProcess(PID)) {
        std::cout << "Process memory released successfully.\n";
    }
    std::cout << "====================================\n";
}

bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.is_open();
}

} // namespace

int main() {
    std::cout << "Initializing VirtualFileSystem...\n";

    const std::string diskPath = "data/virtual_disk.bin";
    FileSystem fs(diskPath);

    if (fileExists(diskPath)) {
        if (fs.mount()) {
            std::cout << "Existing filesystem mounted successfully!\n";
        } else {
            std::cerr << "WARNING: Existing disk image found but mount failed:\n  "
                      << fs.getLastError() << "\n"
                      << "Filesystem not automatically overwritten to prevent data loss.\n"
                      << "Type 'format' to reformat if desired.\n";
        }
    } else {
        std::cout << "No existing virtual disk found. Formatting new disk...\n";
        if (!fs.format()) {
            std::cerr << "ERROR: Failed to format the virtual disk: " << fs.getLastError() << "\n";
            return 1;
        }
        std::cout << "New filesystem formatted successfully!\n";
    }

    Scheduler activeScheduler(2);
    MemoryManager activeMemory(64 * 1024);

    std::cout << "Virtual Operating System Environment ready!\n";
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
        else if (command == "format") {
            std::size_t blocks = DiskManager::DEFAULT_BLOCK_COUNT;
            std::uint32_t inodes = FileSystem::DEFAULT_INODE_COUNT;
            input >> blocks >> inodes;
            if (fs.format(blocks, inodes)) {
                std::cout << "Disk formatted successfully (" << blocks << " blocks, " << inodes << " inodes).\n";
            } else {
                std::cout << "Format failed: " << fs.getLastError() << '\n';
            }
        }
        else if (command == "ls") {
            std::string path = "/";
            input >> path;
            const auto entries = fs.listDirectory(path);
            if (!fs.isMounted()) {
                std::cout << "Filesystem is not mounted.\n";
            } else if (entries.empty()) {
                std::cout << "Directory is empty or not found: " << path << '\n';
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
                std::cout << "Failed to create file: " << fs.getLastError() << '\n';
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
                std::cout << "Failed to create directory: " << fs.getLastError() << '\n';
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
                std::cout << "File written successfully (" << data.size() << " bytes).\n";
            } else {
                std::cout << "Failed to write file: " << fs.getLastError() << '\n';
            }
        }
        else if (command == "read") {
            std::string path;
            input >> path;
            if (path.empty()) {
                std::cout << "Usage: read <path>\n";
            } else if (!fs.exists(path)) {
                std::cout << "File not found: " << path << '\n';
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
                std::cout << "Failed to delete file: " << fs.getLastError() << '\n';
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
                std::cout << "Failed to delete directory: " << fs.getLastError() << '\n';
            }
        }
        else if (command == "info") {
            fs.printInfo();
        }
        else if (command == "cpu") {
            runCpuDemo();
        }
        else if (command == "sched-add") {
            std::string name;
            std::size_t burst = 0;
            int priority = 0;
            std::size_t arrival = 0;
            input >> name >> burst;
            if (input >> priority) {
                input >> arrival;
            }
            if (name.empty() || burst == 0) {
                std::cout << "Usage: sched-add <name> <burst> [priority] [arrival]\n";
            } else {
                const auto pid = activeScheduler.createProcess(name, burst, priority, arrival);
                std::cout << "Process created: PID " << pid << " (" << name << ", burst=" << burst
                          << ", priority=" << priority << ", arrival=" << arrival << ")\n";
            }
        }
        else if (command == "sched-run") {
            std::size_t q = 0;
            if (input >> q && q > 0) {
                activeScheduler.setTimeQuantum(q);
            }
            if (!activeScheduler.hasProcesses()) {
                std::cout << "No processes in scheduler. Use 'sched-add <name> <burst>' or run 'cpu'.\n";
            } else {
                activeScheduler.run();
                printSchedulerResults(activeScheduler);
            }
        }
        else if (command == "sched-clear") {
            activeScheduler.clear();
            std::cout << "Scheduler queue cleared.\n";
        }
        else if (command == "mem") {
            runMemoryDemo();
        }
        else if (command == "mem-alloc") {
            std::uint32_t pid = 0;
            std::size_t bytes = 0;
            if (!(input >> pid >> bytes) || bytes == 0) {
                std::cout << "Usage: mem-alloc <pid> <bytes>\n";
            } else if (activeMemory.allocateProcess(pid, bytes)) {
                std::cout << "Allocated " << bytes << " virtual bytes for PID " << pid << ".\n";
            } else {
                std::cout << "Failed to allocate memory for PID " << pid << ".\n";
            }
        }
        else if (command == "mem-free") {
            std::uint32_t pid = 0;
            if (!(input >> pid)) {
                std::cout << "Usage: mem-free <pid>\n";
            } else if (activeMemory.freeProcess(pid)) {
                std::cout << "Released memory for PID " << pid << ".\n";
            } else {
                std::cout << "Failed to release memory for PID " << pid << " (not found).\n";
            }
        }
        else if (command == "mem-write") {
            std::uint32_t pid = 0;
            std::size_t addr = 0;
            int val = 0;
            if (!(input >> pid >> addr >> val)) {
                std::cout << "Usage: mem-write <pid> <virtual_addr> <value>\n";
            } else if (activeMemory.write(pid, addr, static_cast<std::uint8_t>(val))) {
                std::cout << "Write successful.\n";
            } else {
                std::cout << "Write failed (invalid PID, out of bounds, or memory full).\n";
            }
        }
        else if (command == "mem-read") {
            std::uint32_t pid = 0;
            std::size_t addr = 0;
            if (!(input >> pid >> addr)) {
                std::cout << "Usage: mem-read <pid> <virtual_addr>\n";
            } else {
                std::uint8_t val = 0;
                if (activeMemory.read(pid, addr, val)) {
                    std::cout << "Byte at virtual address " << addr << ": " << static_cast<int>(val) << '\n';
                } else {
                    std::cout << "Read failed (invalid PID or out of bounds).\n";
                }
            }
        }
        else if (command == "mem-map") {
            activeMemory.printMemoryMap();
            std::cout << "Free memory: " << activeMemory.getFreeMemory() << " / "
                      << activeMemory.getPhysicalMemorySize() << " bytes\n"
                      << "Page faults: " << activeMemory.getPageFaultCount() << ", Evictions: "
                      << activeMemory.getPageReplacementCount() << '\n';
        }
        else if (command == "mem-policy") {
            std::string allocP, replP;
            input >> allocP >> replP;
            if (allocP == "demand") {
                activeMemory.setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
                std::cout << "Allocation policy set to DEMAND_PAGING.\n";
            } else if (allocP == "eager") {
                activeMemory.setAllocationPolicy(AllocationPolicy::EAGER);
                std::cout << "Allocation policy set to EAGER.\n";
            }
            if (replP == "lru") {
                activeMemory.setReplacementPolicy(ReplacementPolicy::LRU);
                std::cout << "Replacement policy set to LRU.\n";
            } else if (replP == "fifo") {
                activeMemory.setReplacementPolicy(ReplacementPolicy::FIFO);
                std::cout << "Replacement policy set to FIFO.\n";
            }
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
