# VirtualFileSystem

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/build-CMake%20%7C%20Make-informational.svg)
![Warnings](https://img.shields.io/badge/flags--Wall%20--Wextra%20--Wpedantic-clean-success.svg)

A user-space mini operating system written in modern **C++17**. It combines three classic OS building blocks in one interactive shell:

- a **persistent, block-based filesystem** stored in a single binary disk image
- a **Round-Robin CPU scheduler** with Gantt charts and performance metrics
- a **virtual memory manager** with paging, demand paging, and FIFO/LRU page replacement

It is built as a learning and demonstration project: every subsystem is small enough to read end to end, yet implements the real mechanisms (inodes, bitmaps, page tables, quanta, eviction) rather than faking them.

---

## Table of Contents

- [Features](#features)
- [Quick Start](#quick-start)
- [Building](#building)
- [Running the Tests](#running-the-tests)
- [Interactive Shell Reference](#interactive-shell-reference)
- [Example Session](#example-session)
- [Architecture at a Glance](#architecture-at-a-glance)
- [Project Structure](#project-structure)
- [Tech Stack and Standards](#tech-stack-and-standards)

---

## Features

### 1. Persistent Block-Based Filesystem

| Capability | Details |
| --- | --- |
| **Disk image** | Backed by a binary file. Geometry is synchronized with the formatted layout. Valid images are mounted and restored; corrupted images are rejected rather than silently overwritten. |
| **On-disk metadata** | Versioned superblock, multi-block inode and data-block bitmaps, an inode table (32 inodes per block), and directory entries stored in allocated data blocks. |
| **Hierarchical directories** | Arbitrary nesting such as `/projects/VirtualOS/src/main.cpp`. Paths are normalized, so redundant or trailing slashes like `///a//b/` resolve correctly. Non-empty directories and the root cannot be deleted. |
| **Multi-block files** | Empty and multi-block files up to **4,243,456 bytes** (about 4.05 MiB) using 12 direct block pointers plus 1 single-indirect block (1024 block references). |
| **Operation safety** | Staged allocation with automatic rollback on disk write failure or storage exhaustion, so blocks never leak and metadata is not corrupted. |
| **Consistency checks** | Mount-time audit of magic number, version, region boundaries, bitmap-to-inode agreement, leak-free block reference counts, and directory integrity. |

### 2. CPU Virtualization and Scheduling

- **Round-Robin scheduling** with a configurable time quantum.
- **Gantt chart and execution log** showing timeline slices and run order.
- **Per-process tracking** of PID, state (`NEW`, `READY`, `RUNNING`, `BLOCKED`, `TERMINATED`), CPU burst, arrival time, completion time, waiting time, turnaround time, and initial response time.
- **Aggregate metrics**: average waiting time, average turnaround time, and CPU utilization percentage.
- **Repeatable and interactive**: workloads can be re-run, duplicate scheduling is rejected, and custom processes can be added from the shell.

### 3. Virtual Memory

- **Paged memory** with configurable physical frames (default 64 KiB physical memory, 4 KiB pages) and a page table per process.
- **Address translation** with bounds checking, integer-overflow safeguards, and write-permission verification.
- **Allocation policies**: eager allocation or **demand paging**.
- **Page replacement**: configurable **FIFO** or **LRU** when physical frames run out.
- **Diagnostics**: cumulative page faults, frame evictions, and a live frame allocation map.

---

## Quick Start

```bash
git clone https://github.com/anuragpatil1729/FileSystem.git
cd FileSystem

cmake -S . -B build
cmake --build build

./build/vfs
```

Then try `info`, `mkdir /demo`, `cpu`, and `mem` at the prompt.

---

## Building

**Requirements:** a C++17 compiler (GCC, Clang, or MSVC) and either CMake 3.16+ or GNU Make.

### With CMake

```bash
cmake -S . -B build
cmake --build build
```

### With GNU Make

```bash
make
```

Both produce the interactive shell at `./build/vfs`.

---

## Running the Tests

### CMake

```bash
ctest --test-dir build --output-on-failure
```

### Make

```bash
make test
```

The test suite covers:

| Test file | What it verifies |
| --- | --- |
| `tests/test_disk.cpp` | Block I/O, resizing, fault injection |
| `tests/test_filesystem.cpp` | Directory hierarchy, multi-block files, persistence, rollback |
| `tests/test_memory.cpp` | Address translation, demand paging, FIFO and LRU replacement |
| `tests/test_scheduler.cpp` | Round-Robin behavior, arrival times, metrics |

---

## Interactive Shell Reference

Start the shell with `./build/vfs`.

### Filesystem

| Command | Description |
| --- | --- |
| `ls [path]` | List a directory, e.g. `ls /` or `ls /projects` |
| `create <path>` | Create an empty file |
| `mkdir <path>` | Create a directory |
| `write <path> <text>` | Write text (multi-block capable) to a file |
| `read <path>` | Print a file's contents |
| `rm <path>` | Delete a file and reclaim its blocks and inode |
| `rmdir <path>` | Delete an empty directory |
| `info` | Show geometry, allocated and free blocks, and inodes |
| `format [blocks] [inodes]` | Format the disk image with custom geometry |

### CPU Scheduler

| Command | Description |
| --- | --- |
| `cpu` | Run the demo with a predefined workload |
| `sched-add <n> <burst> [priority] [arrival]` | Add a process to the scheduler |
| `sched-run [quantum]` | Run the scheduler and print the Gantt chart and statistics |
| `sched-clear` | Clear the ready queue |

### Virtual Memory

| Command | Description |
| --- | --- |
| `mem` | Run the demand-paging demonstration |
| `mem-alloc <pid> <bytes>` | Allocate virtual memory for a process |
| `mem-free <pid>` | Free a process's frames and page table |
| `mem-write <pid> <addr> <val>` | Write a byte to a virtual address |
| `mem-read <pid> <addr>` | Read a byte from a virtual address |
| `mem-map` | Print the frame allocation map and fault statistics |
| `mem-policy <eager\|demand> [fifo\|lru]` | Switch allocation and replacement policies |

### General

| Command | Description |
| --- | --- |
| `exit` | Safely unmount the filesystem and quit |

---

## Example Session

```text
vfs> mkdir /projects
vfs> mkdir /projects/VirtualOS
vfs> create /projects/README.md
vfs> write /projects/README.md Hello from the virtual disk!
vfs> read /projects/README.md
vfs> ls /projects
vfs> info

vfs> sched-add 1 8
vfs> sched-add 2 5
vfs> sched-run 3

vfs> mem-policy demand lru
vfs> mem-alloc 1 16384
vfs> mem-write 1 100 42
vfs> mem-read 1 100
vfs> mem-map

vfs> exit
```

Because the filesystem is persistent, files created in one session are still there the next time you launch the shell.

---

## Architecture at a Glance

```
            +-------------------------------+
            |   Interactive CLI  (main.cpp) |
            +---------------+---------------+
                            |
      +---------------------+----------------------+
      |                     |                      |
+-----v------+      +-------v--------+     +-------v--------+
| Filesystem |      |   Scheduler    |     | MemoryManager  |
| (inodes,   |      | (Round-Robin,  |     | (page tables,  |
| dirs,      |      |  metrics,      |     |  demand paging,|
| bitmaps)   |      |  Gantt)        |     |  FIFO / LRU)   |
+-----+------+      +----------------+     +----------------+
      |
+-----v------+
| DiskManager|  block I/O, geometry detection, fault injection
+-----+------+
      |
  binary disk image
```

For the full design specification, see [`docs/architecture.md`](docs/architecture.md).

---

## Project Structure

```text
VirtualFileSystem/
├── include/
│   ├── bitmap.hpp          # Bitmap allocator with raw byte access
│   ├── byte_io.hpp         # Portable little-endian binary serialization
│   ├── directory.hpp       # Binary directory serialization and entry table
│   ├── disk_manager.hpp    # Block I/O, geometry detection, fault injection
│   ├── filesystem.hpp      # Filesystem core, path resolution, multi-block files
│   ├── inode.hpp           # 128-byte inode with direct and indirect blocks
│   ├── memory_manager.hpp  # Paged memory, demand paging, FIFO/LRU replacement
│   ├── page_table.hpp      # Per-process page table and entry metadata
│   ├── process.hpp         # Simulated process representation and state
│   ├── scheduler.hpp       # Round-Robin scheduler and performance metrics
│   └── superblock.hpp      # Filesystem geometry and layout validator
├── src/                    # Implementations plus main.cpp (interactive shell)
├── tests/                  # Disk, filesystem, memory, and scheduler tests
├── docs/
│   └── architecture.md     # In-depth architectural design specification
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

## Tech Stack and Standards

- **Language:** C++17, compiled with `-Wall -Wextra -Wpedantic`
- **Build systems:** CMake 3.16+ and GNU Make
- **Design:** portable little-endian binary serialization, RAII resource management, and no raw owning pointers `exit` : Safely unmount filesystem and quit.
