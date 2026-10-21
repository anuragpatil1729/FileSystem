# VFS Studio - Qt C++ Graphical User Interface

A modern desktop GUI for **VirtualFileSystem**, implemented in **C++17** and **Qt 6**. It provides an interactive interface for all three core operating system modules without modifying any of the original codebase or virtual disk state.

---

## Highlights & Features

### 1. 🗄️ Filesystem Explorer
- **Directory Browser**: Hierarchical navigation (`/`, `/projects`, etc.) with parent directory traversal.
- **File Management**: Create files (`create`), make directories (`mkdir`), delete files and folders (`rm`, `rmdir`).
- **Integrated File Viewer & Editor**: Click any file to view and edit its text contents in real-time and save directly back to the virtual disk.
- **Disk Geometry & Storage Gauges**: Live visual progress bars and status indicators for:
  - Block usage (`used / total`, % free)
  - Inode allocation (`used / total`)
  - Superblock layout parameters (data block start, block size, max file size)
- **Disk Reformatting Dialog**: Option to format the disk with custom block and inode counts.

### 2. ⚡ CPU Scheduler & Gantt Chart
- **Configurable Quantum**: Dynamic Round-Robin time quantum configuration.
- **Process Creator**: Add custom processes with Name, CPU Burst Time, Priority, and Arrival Time.
- **Predefined Workload**: 1-click loading and execution of demo workloads.
- **Interactive Visual Gantt Chart**: Custom-painted timeline widget displaying colored time slices and process execution intervals (`P1 [0-2]`, etc.).
- **Process Metrics Table**: Detailed breakdown of Arrival, Completion, Waiting, Turnaround, and Response times.
- **KPI Summary Cards**: Average Waiting Time, Average Turnaround Time, and CPU Utilization percentage.

### 3. 🧠 Virtual Memory & Demand Paging
- **Policy Selectors**:
  - Allocation: `DEMAND_PAGING` vs `EAGER`
  - Replacement: `LRU` vs `FIFO`
- **Process Virtual Memory Manager**: Allocate and free virtual memory per PID.
- **Address Translation & Memory Simulator**:
  - Write bytes to virtual addresses (handles and logs page faults).
  - Read bytes from virtual addresses.
  - Translate Virtual Address to Physical Frame + Offset.
- **Physical Memory Frame Visualizer**: Real-time visual 16-frame grid showing which physical frames are FREE vs USED by PID and Virtual Page number.
- **Live Diagnostics**: Free physical memory bar, Cumulative Page Fault counter, and Eviction/Replacement counter.

### 4. 💻 Interactive VFS Shell
- **Embedded Console**: Run all native VFS commands (`help`, `ls`, `mkdir`, `read`, `write`, `info`, `cpu`, `sched-run`, `mem`, `mem-map`, etc.).
- **Quick Action Chips**: 1-click execution for common inspection commands.
- **Command History**: Browse previous commands using the Up and Down arrow keys.
- **Real-Time Synchronization**: Any changes made in the terminal immediately update the Filesystem, Scheduler, and Memory tabs.

---

## Building and Running

### Prerequisites
- Qt 6 (installed via `brew install qt` or standard package manager)
- CMake 3.16+
- C++17 compiler (Clang / GCC)

### Quick Run
From the repository root:

```bash
./gui/run.sh
```

### Manual Build
```bash
cmake -S gui -B gui/build
cmake --build gui/build

# Run executable from root:
./gui/build/vfs_gui
```
