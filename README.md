# VirtualFileSystem (VFS) & OS Simulation

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Qt 6](https://img.shields.io/badge/Qt-6.x%20Widgets-41CD52.svg)](https://www.qt.io/)
[![Build](https://img.shields.io/badge/build-CMake%20%7C%20Make-informational.svg)]()
[![Tests](https://img.shields.io/badge/tests-100%25%20passing%20(4%2F4)-success.svg)]()
[![Warnings](https://img.shields.io/badge/flags--Wall%20--Wextra%20--Wpedantic-clean-success.svg)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg)]()

A comprehensive, user-space mini operating system written in modern **C++17** and featuring both an **Interactive CLI** and a desktop **Qt 6 GUI Studio**.

VFS implements three foundational operating system subsystems from first principles without external simulation shortcuts:

1. **Persistent Block-Based Filesystem**: Stored in a single binary disk image (`data/virtual_disk.bin`), with a versioned superblock, bitmap allocators, 128-byte inodes, direct & indirect block addressing, hierarchical directories, and transactional rollback on allocation failures.
2. **Round-Robin CPU Scheduler**: Preemptive time-sliced CPU virtualization with process lifecycle tracking, visual Gantt chart timelines, execution logs, and full performance accounting (Waiting, Turnaround, Response times, and CPU Utilization).
3. **Virtual Memory Manager**: Paged virtual memory with 4 KiB frames, per-process page tables, address translation, **demand paging**, page fault handling, and configurable **FIFO** or **LRU** frame eviction policies.

---

## Table of Contents

- [Subsystem Highlights](#subsystem-highlights)
- [System Architecture](#system-architecture)
- [On-Disk Filesystem Layout](#on-disk-filesystem-layout)
- [Desktop GUI (VFS Studio)](#desktop-gui-vfs-studio)
- [Quick Start](#quick-start)
- [Building the Project](#building-the-project)
- [Running the Test Suite](#running-the-test-suite)
- [Interactive Shell Command Reference](#interactive-shell-command-reference)
- [Complete Walkthrough Session](#complete-walkthrough-session)
- [Project Directory Structure](#project-directory-structure)
- [Engineering Standards & Design Principles](#engineering-standards--design-principles)

---

## Subsystem Highlights

### 1. Persistent Block-Based Filesystem

| Feature | Description |
| :--- | :--- |
| **Binary Disk Image** | Backed by a single binary container. Valid formatted layouts are mounted upon launch; corrupted images are detected and safely rejected to avoid data loss. |
| **On-Disk Metadata** | Layout is computed deterministically from geometry: superblock (Block 0), inode bitmap, block bitmap, inode table (32 records per block), and data blocks. |
| **Hierarchical Directories** | Arbitrary folder nesting (e.g. `/projects/os/src/main.cpp`). Normalized paths safely resolve redundant slashes (`///a//b/`). Non-empty directories and the root cannot be deleted. |
| **Multi-Block Files** | Supports file sizes up to **4,243,456 bytes (~4.05 MiB)** using 12 direct block pointers plus 1 single-indirect block addressing table (1024 data block pointers). |
| **Transactional Safety** | Multi-block allocations follow a two-phase staged reservation. On write failure or storage exhaustion, allocations automatically roll back so blocks and inodes never leak. |
| **Mount-Time Audit** | Validates magic numbers (`0x56534653`), version boundaries, bitmap-to-inode consistency, and leak-free reference counts before mounting. |

### 2. CPU Virtualization & Scheduling

- **Round-Robin Scheduling**: Preemptive execution based on a configurable time quantum.
- **Gantt Chart & Execution Sequence**: Exact tracking of process time slices (`| P1 [0-2] | P2 [2-4] |`) and execution ordering.
- **Per-Process Metrics**:
  - PID and process name
  - Execution state (`NEW`, `READY`, `RUNNING`, `BLOCKED`, `TERMINATED`)
  - CPU burst time, priority, and arrival time
  - Completion time, waiting time, turnaround time, and initial response time
- **Aggregate KPI Accounting**: Average waiting time, average turnaround time, total clock units, and real-time CPU utilization percentage.
- **Workload Management**: Clear, configure, add custom processes, or run standard benchmark workloads (`cpu`).

### 3. Paged Virtual Memory & Demand Paging

- **Paged Physical Memory**: Default 64 KiB physical RAM divided into 16 discrete 4 KiB frames.
- **Per-Process Page Tables**: Virtual-to-physical address translation with page presence, dirty bits, write permissions, and access timestamps.
- **Configurable Allocation Policies**:
  - **Demand Paging**: Virtual pages are allocated on first read/write access via simulated page fault interrupts.
  - **Eager Allocation**: All virtual pages are allocated physical frames immediately upon process creation.
- **Frame Replacement Policies**:
  - **LRU (Least Recently Used)**: Evicts the frame with the oldest access timestamp.
  - **FIFO (First In, First Out)**: Evicts the frame allocated earliest in time.
- **Diagnostics**: Cumulative page fault counter, frame eviction counter, and physical frame allocation mapping.

---

## System Architecture

```text
                        +------------------------------------+
                        |       Application Interfaces       |
                        |   Interactive CLI  |  Qt 6 GUI     |
                        +-----------------+------------------+
                                          |
          +-------------------------------+-------------------------------+
          |                               |                               |
          v                               v                               v
+-------------------+           +-------------------+           +-------------------+
|    Filesystem     |           |     Scheduler     |           |   MemoryManager   |
| (Inodes, Bitmaps, |           |   (Round-Robin,   |           |  (Page Tables,    |
| Hierarchical Dirs)|           |  Gantt, Metrics)  |           |   Demand Paging,  |
+---------+---------+           +---------+---------+           |   FIFO / LRU)     |
          |                               |                     +---------+---------+
          v                               v                               |
+-------------------+           +-------------------+                     v
|    DiskManager    |           |    Simulated      |           +-------------------+
|  (Block I/O,      |           |    Processes      |           |  Physical Memory  |
| Fault Injection)  |           +-------------------+           |  Frames (16x4KB)  |
+---------+---------+                                           +-------------------+
          |
          v
  virtual_disk.bin
```

---

## On-Disk Filesystem Layout

The virtual disk is divided into fixed 4096-byte blocks formatted according to the **VSFS v2** layout specification:

```text
+--------------+------------------+------------------+------------------+-----------------------+
|   Block 0    |  Blocks 1 .. S1  | Blocks S1+1 .. S2| Blocks S2+1 .. S3|  Blocks S3+1 .. End   |
|  Superblock  |   Inode Bitmap   |   Block Bitmap   |   Inode Table    |      Data Blocks      |
+--------------+------------------+------------------+------------------+-----------------------+
```

### Inode Addressing Scheme

Each on-disk inode record is **128 bytes**, accommodating:
- File metadata: Inode type (File / Directory), size in bytes, timestamps, link count.
- **12 Direct Block Pointers**: Directly reference blocks 0 through 11 (up to 48 KiB).
- **1 Single-Indirect Pointer**: References an indirect index block containing 1024 32-bit block addresses (up to 4 MiB).
- **Total Capacity**: $12 + 1024 = 1036 \text{ blocks} = 4,243,456 \text{ bytes}$.

---

## Desktop GUI (VFS Studio)

A complete desktop application built in **C++17 and Qt 6** is included in the `gui/` directory:

| Tab | Capabilities |
| :--- | :--- |
| **🗄️ Filesystem Explorer** | Tree & list navigation of folders (`/`, `/projects`), real-time file creation (`create`), directory creation (`mkdir`), deletion (`rm`/`rmdir`), integrated text editor to read and edit file contents directly on disk, and storage usage gauges for blocks & inodes. |
| **⚡ CPU Scheduler** | Configurable time quantum, interactive process queue editor, **custom-painted Gantt chart widget**, execution sequence logger, process metrics table, and KPI cards (Waiting, Turnaround, CPU Utilization). |
| **🧠 Virtual Memory** | Demand paging vs. Eager policy switch, LRU vs. FIFO selector, virtual memory allocator, byte-level read/write tester with page fault detection, and a **visual 16-frame physical memory allocation grid**. |
| **💻 Interactive Shell** | Embedded terminal console with prompt `vfs>`, command history navigation (Up/Down arrow keys), quick action chips, and full two-way synchronization with GUI tabs. |

---

## Quick Start

### 1. Interactive CLI Shell

```bash
# Clone the repository
git clone https://github.com/anuragpatil1729/FileSystem.git
cd FileSystem

# Build and run with CMake
cmake -S . -B build
cmake --build build
./build/vfs
```

Try these commands inside the shell:
```text
vfs> info
vfs> mkdir /demo
vfs> write /demo/hello.txt Welcome to VirtualFileSystem!
vfs> read /demo/hello.txt
vfs> cpu
vfs> mem
vfs> exit
```

### 2. Desktop GUI Studio

```bash
# Run the automated build & launch script
./gui/run.sh
```

Or build manually:
```bash
cmake -S gui -B gui/build
cmake --build gui/build
./gui/build/vfs_gui
```

---

## Building the Project

### Requirements
- **C++ Compiler**: Clang 11+, GCC 9+, or MSVC supporting C++17
- **Build System**: CMake 3.16+ or GNU Make
- **GUI (Optional)**: Qt 6 (`qtbase`, `QtWidgets`) — installable via `brew install qt` on macOS or `apt install qt6-base-dev` on Linux.

### Building with CMake
```bash
cmake -S . -B build
cmake --build build
```

### Building with GNU Make
```bash
make
```

Both build targets generate the interactive CLI binary at `./build/vfs`.

---

## Running the Test Suite

The test suite validates disk block I/O, crash safety, filesystem persistence, Round-Robin scheduling invariants, and virtual memory paging correctness:

### Via CTest (CMake)
```bash
ctest --test-dir build --output-on-failure
```

### Via Make
```bash
make test
```

### Test Coverage Breakdown

| Test Binary | Target Tested | Verification Scope |
| :--- | :--- | :--- |
| `test_disk` | `DiskManager` | Block read/write integrity, disk image resizing, out-of-bounds safety, and simulated fault injection. |
| `test_filesystem` | `FileSystem` | Deep folder nesting, path normalization, multi-block files (direct + indirect), mount audits, and allocation rollback on failure. |
| `test_scheduler` | `Scheduler` | Round-Robin execution order, time quantum preemption, arrival time interleaving, and turnaround/waiting math. |
| `test_memory` | `MemoryManager` | Virtual address translation, demand paging page faults, dirty bit tracking, and FIFO vs. LRU victim frame replacement. |

---

## Interactive Shell Command Reference

Start the shell using `./build/vfs`.

### Filesystem Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `ls` | `[path]` | List entries in a directory (defaults to `/`) | `ls /projects` |
| `create` | `<path>` | Create an empty regular file | `create /notes.txt` |
| `mkdir` | `<path>` | Create a new directory | `mkdir /projects/os` |
| `write` | `<path> <text>` | Write text to a file (allocates direct/indirect blocks) | `write /notes.txt Hello VFS!` |
| `read` | `<path>` | Output the text content of a file | `read /notes.txt` |
| `rm` | `<path>` | Delete a file and free its allocated blocks and inode | `rm /notes.txt` |
| `rmdir` | `<path>` | Delete an empty directory | `rmdir /projects/os` |
| `info` | — | Display block geometry, free blocks, and free inodes | `info` |
| `format` | `[blocks] [inodes]` | Reformat virtual disk with custom geometry | `format 2048 256` |

### CPU Scheduler Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `cpu` | — | Run standard benchmark workload with Gantt chart output | `cpu` |
| `sched-add` | `<name> <burst> [prio] [arr]` | Add a process to the ready queue | `sched-add Worker 6 1 0` |
| `sched-run` | `[quantum]` | Execute the scheduler with configurable time quantum | `sched-run 2` |
| `sched-clear` | — | Flush and clear the ready queue | `sched-clear` |

### Virtual Memory Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `mem` | — | Run the automated demand-paging demonstration | `mem` |
| `mem-alloc` | `<pid> <bytes>` | Allocate virtual address space for a process | `mem-alloc 1 16384` |
| `mem-free` | `<pid>` | Release all physical frames and page table for a process | `mem-free 1` |
| `mem-write` | `<pid> <addr> <val>` | Write a byte (0-255) to a virtual address | `mem-write 1 4200 171` |
| `mem-read` | `<pid> <addr>` | Read a byte from a virtual address | `mem-read 1 4200` |
| `mem-map` | — | Display physical frame allocation grid and fault metrics | `mem-map` |
| `mem-policy` | `<eager\|demand> [fifo\|lru]` | Configure allocation and replacement policies | `mem-policy demand lru` |

### General Commands

| Command | Description |
| :--- | :--- |
| `help` | Display interactive command reference |
| `exit` | Safely unmount the filesystem, sync metadata, and quit |

---

## Complete Walkthrough Session

```text
$ ./build/vfs
Initializing VirtualFileSystem...
Existing filesystem mounted successfully!
Virtual Operating System Environment ready!

vfs> mkdir /projects
vfs> mkdir /projects/os
vfs> create /projects/os/kernel.txt
vfs> write /projects/os/kernel.txt Microkernel with paged memory and RR scheduling
File written successfully (52 bytes).

vfs> read /projects/os/kernel.txt
Microkernel with paged memory and RR scheduling

vfs> ls /projects/os
kernel.txt

vfs> info
===== Virtual File System =====
Block size: 4096 bytes
Total blocks: 1024
Free blocks: 1011
Total inodes: 128
Free inodes: 121
Data block start: 7
Max file size: 4243456 bytes (4144 KiB)
Mounted: yes
===============================

vfs> sched-add TaskA 5 1 0
Process created: PID 1 (TaskA, burst=5, priority=1, arrival=0)

vfs> sched-add TaskB 3 1 0
Process created: PID 2 (TaskB, burst=3, priority=1, arrival=0)

vfs> sched-run 2
========== CPU SCHEDULER ==========
Algorithm: Round Robin (Simulated CPU Time)
Time quantum: 2 simulated time units

Gantt Timeline:
| P1 [0-2] | P2 [2-4] | P1 [4-6] | P2 [6-7] | P1 [7-8] |

Execution order: P1 P2 P1 P2 P1 

Process Statistics (Simulated CPU Clocks):
PID     Name        Burst   Arrival   Completion    Waiting   Turnaround  Response  
P1      TaskA       5       0         8             3         8           0         
P2      TaskB       3       0         7             4         7           2         

Average waiting time: 3.50
Average turnaround time: 7.50
CPU Utilization: 100.00%
Total simulated CPU time: 8 units
===================================

vfs> mem-policy demand lru
Allocation policy set to DEMAND_PAGING.
Replacement policy set to LRU.

vfs> mem-alloc 1 12288
Allocated 12288 virtual bytes for PID 1.

vfs> mem-write 1 4200 171
Write successful.

vfs> mem-read 1 4200
Byte at virtual address 4200: 171

vfs> mem-map
===== Memory Map =====
Frame 0: USED (PID 1, VPage 1)
Frame 1: FREE
...
Frame 15: FREE
======================
Free memory: 61440 / 65536 bytes
Page faults: 1, Evictions: 0

vfs> exit
Unmounting filesystem...
Goodbye!
```

---

## Project Directory Structure

```text
VirtualFileSystem/
├── include/                   # Public subsystem C++ API headers
│   ├── bitmap.hpp             # In-memory & block-aligned bitmap allocator
│   ├── byte_io.hpp            # Endian-neutral little-endian binary serialization
│   ├── directory.hpp          # Binary directory records serialization
│   ├── disk_manager.hpp       # Block device abstraction and fault injector
│   ├── filesystem.hpp         # Filesystem core, path normalization, multi-block files
│   ├── inode.hpp              # 128-byte inode record (direct & indirect blocks)
│   ├── memory_manager.hpp     # Virtual memory, demand paging, FIFO/LRU policies
│   ├── page_table.hpp         # Per-process page table and entry metadata
│   ├── process.hpp            # Process state machine and PCB representation
│   ├── scheduler.hpp          # Round-Robin scheduler and Gantt statistics
│   └── superblock.hpp         # VSFS v2 superblock geometry & layout validator
│
├── src/                       # Core implementations and CLI main entrypoint
│   ├── bitmap.cpp
│   ├── directory.cpp
│   ├── disk_manager.cpp
│   ├── filesystem.cpp
│   ├── inode.cpp
│   ├── memory_manager.cpp
│   ├── page_table.cpp
│   ├── process.cpp
│   ├── scheduler.cpp
│   ├── superblock.cpp
│   └── main.cpp               # Interactive CLI terminal shell
│
├── gui/                       # Qt 6 Desktop GUI Studio (C++17)
│   ├── CMakeLists.txt         # Independent GUI build configuration
│   ├── run.sh                 # One-click GUI build & launch script
│   ├── main.cpp               # Qt application entrypoint
│   ├── mainwindow.hpp/.cpp    # Main window with tabs, dark theme, and status bar
│   ├── filesystem_tab.hpp/.cpp# Filesystem explorer & text editor widget
│   ├── scheduler_tab.hpp/.cpp # CPU scheduler & metrics dashboard
│   ├── memory_tab.hpp/.cpp    # Virtual memory & paging simulator
│   ├── terminal_tab.hpp/.cpp  # Embedded interactive shell console
│   ├── gantt_widget.hpp/.cpp  # Custom-painted timeline Gantt chart
│   └── frame_map_widget.hpp/.cpp # Custom physical frame allocation grid
│
├── tests/                     # Automated unit and integration test suite
│   ├── test_disk.cpp          # Block I/O and fault resilience tests
│   ├── test_filesystem.cpp    # Hierarchy, persistence, and rollback tests
│   ├── test_scheduler.cpp     # Preemption, metrics, and quantum tests
│   └── test_memory.cpp        # Address translation, paging, and replacement tests
│
├── data/                      # Persistent virtual disk container
│   └── virtual_disk.bin       # Binary formatted virtual disk image
│
├── docs/                      # Architectural specifications
│   └── architecture.md        # Technical architecture documentation
│
├── CMakeLists.txt             # Primary build configuration (CLI + Tests)
├── Makefile                   # GNU Make build alternative
└── README.md                  # Comprehensive project documentation
```

---

## Engineering Standards & Design Principles

- **Modern C++17**: Standardized idioms (`std::optional`, `std::unique_ptr`, `std::string_view`, structured bindings).
- **Strict Compiler Warnings**: Builds with zero warnings under `-Wall -Wextra -Wpedantic`.
- **Zero Raw Owning Pointers**: Full RAII memory management across all disk, process, and memory management subsystems.
- **Endian-Independent Serialization**: Multi-byte integers serialize in explicit little-endian format, ensuring binary disk images are portable across x86_64, ARM64, and other architectures.
- **Deterministic Crash Rollback**: Staged allocation ensures metadata consistency is never compromised by incomplete writes or exhausted disk capacity.
