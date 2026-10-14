# VirtualFileSystem

A robust, functional C++17 user-space operating system environment featuring a persistent block-based filesystem, a Round-Robin CPU process scheduler with metrics, and a virtual memory manager with demand paging and page replacement.

## Key Features

### 1. Persistent Block-Based Virtual Filesystem
- **Disk Geometry & Safe Mounting**: Backed by a binary disk file. The active disk manager synchronizes geometry with the formatted layout. Existing valid images are mounted and restored; corrupted images are safely rejected without silent overwrites.
- **Persistent Metadata**: On-disk versioned Superblock, multi-block Inode and Data-block Bitmaps, Inode Table (32 inodes/block), and directory entries stored in allocated data blocks.
- **Hierarchical Directory Tree**: Full path resolution supporting arbitrary directory nesting (e.g., `/projects/VirtualOS/src/main.cpp`). Normalized path traversal handles redundant/trailing slashes (`///a//b/`). Deletion safety enforces non-empty directory protection and prevents root deletion.
- **Multi-Block File Storage**: Supports empty files and multi-block files up to **4,243,456 bytes** (4.047 MiB) using 12 direct block pointers and 1 single-indirect block pointer (1024 block references).
- **Operation Safety & Rollback**: Staged allocation with automatic rollback on disk write failures or storage exhaustion, preventing block leaks and metadata corruption.
- **Metadata Consistency Checks**: Mount-time auditing validates magic, version, region boundaries, bitmap-to-inode consistency, leak-free block reference counting, and directory integrity.

### 2. CPU Virtualization & Scheduling
- **Round-Robin Scheduling**: Preemptive execution tracking using configurable time quanta.
- **Gantt Chart & Execution Logging**: Visual timeline slices and process execution orders.
- **Process Metrics & Workloads**: Tracks PID, state (`NEW`, `READY`, `RUNNING`, `BLOCKED`, `TERMINATED`), CPU burst, arrival time, completion time, waiting time, turnaround time, and initial response time.
- **Average Metrics & Utilization**: Computes average waiting time, average turnaround time, and CPU utilization percentage.
- **Repeatable & Interactive**: Re-executable workloads, duplicate scheduling rejection, and interactive CLI commands to create and run custom processes.

### 3. Virtual Memory Virtualization
- **Paged Memory**: Configurable physical memory frames (default 64 KiB with 4 KiB page size) and per-process page tables.
- **Virtual-to-Physical Address Translation**: Translation bounds checking, integer overflow safeguards, and write-permission verification.
- **Allocation Policies**: Supports both eager allocation and **Demand Paging**.
- **Page Replacement Algorithms**: Implements configurable **FIFO** (First-In, First-Out) and **LRU** (Least Recently Used) page replacement when physical frames are exhausted.
- **Diagnostics & Metrics**: Tracks cumulative page faults, frame evictions/replacements, and provides real-time frame allocation maps.

---

## Tech Stack & Standards

- **Language**: C++17 (`-Wall -Wextra -Wpedantic`)
- **Build Systems**: CMake 3.16+ and GNU Make
- **Architecture**: Portable little-endian binary serialization (`byte_io.hpp`), RAII resource management, zero raw owning pointers.

---

## Project Structure

```text
VirtualFileSystem/
├── include/
│   ├── bitmap.hpp          # Bitmap allocator with raw byte access
│   ├── byte_io.hpp         # Portable little-endian binary serialization
│   ├── directory.hpp       # Binary directory serialization and entry table
│   ├── disk_manager.hpp    # Block I/O, geometry detection, and fault injection
│   ├── filesystem.hpp      # Filesystem core, path resolution, multi-block files
│   ├── inode.hpp           # 128-byte inode format with direct/indirect blocks
│   ├── memory_manager.hpp  # Paged memory, demand paging, FIFO/LRU replacement
│   ├── page_table.hpp      # Per-process page table and entry metadata
│   ├── process.hpp         # Simulated process representation and state
│   ├── scheduler.hpp       # Round-Robin scheduler and performance metrics
│   └── superblock.hpp      # Filesystem geometry and structural layout validator
├── src/
│   ├── bitmap.cpp
│   ├── directory.cpp
│   ├── disk_manager.cpp
│   ├── filesystem.cpp
│   ├── inode.cpp
│   ├── main.cpp            # Integrated interactive CLI shell
│   ├── memory_manager.cpp
│   ├── page_table.cpp
│   ├── process.cpp
│   ├── scheduler.cpp
│   └── superblock.cpp
├── tests/
│   ├── test_disk.cpp       # Block I/O, resizing, fault injection tests
│   ├── test_filesystem.cpp # Hierarchy, multi-block, persistence, rollback tests
│   ├── test_memory.cpp     # Translation, demand paging, FIFO & LRU tests
│   └── test_scheduler.cpp  # Round-robin, arrival times, metrics tests
├── docs/
│   └── architecture.md     # In-depth architectural design specification
├── CMakeLists.txt
├── Makefile
├── README.md
└── .gitignore
```

---

## Building and Testing

### Build with CMake

```bash
cmake -S . -B build
cmake --build build
```

Run test suite:
```bash
ctest --test-dir build --output-on-failure
```

Launch application:
```bash
./build/vfs
```

### Build with GNU Make

```bash
make
make test
./build/vfs
```

---

## Interactive CLI Commands

Start the environment with `./build/vfs`:

### Filesystem Commands
- `ls [path]` : List entries in directory (e.g., `ls /`, `ls /projects`).
- `create <path>` : Create an empty file (e.g., `create /projects/README.md`).
- `mkdir <path>` : Create a new directory (e.g., `mkdir /projects/VirtualOS`).
- `write <path> <text>` : Write multi-block text data to a file.
- `read <path>` : Read and display file contents.
- `rm <path>` : Delete a file and reclaim its data blocks and inode.
- `rmdir <path>` : Delete an empty directory.
- `info` : Display filesystem geometry, allocated/free blocks, and inodes.
- `format [blocks] [inodes]` : Format the virtual disk image with custom geometry.

### CPU Scheduler Commands
- `cpu` : Run CPU scheduler demo with predefined workload.
- `sched-add <name> <burst> [priority] [arrival]` : Add process to active scheduler.
- `sched-run [quantum]` : Execute scheduler and view Gantt chart and statistics.
- `sched-clear` : Clear scheduler ready queue.

### Virtual Memory Commands
- `mem` : Run virtual memory demonstration with demand paging.
- `mem-alloc <pid> <bytes>` : Allocate virtual memory for a process.
- `mem-free <pid>` : Free physical frames and page table for a process.
- `mem-write <pid> <addr> <val>` : Write byte value to process virtual address.
- `mem-read <pid> <addr>` : Read byte value from process virtual address.
- `mem-map` : Print physical frame allocation map and fault statistics.
- `mem-policy <eager|demand> [fifo|lru]` : Switch allocation and replacement policies.

- `exit` : Safely unmount filesystem and quit.
