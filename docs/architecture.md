
# VirtualFileSystem Architecture

## 1. Overview

VirtualFileSystem is a user-space simulation of selected operating
system concepts. It combines a block-based virtual disk, basic
filesystem structures, a CPU scheduler, and paged memory management.

## 2. High-level architecture

```text
                    CLI (main.cpp)
                          |
             +------------+------------+
             |            |            |
             v            v            v
         FileSystem    Scheduler   MemoryManager
             |            |            |
      +------+------+   Process     PageTable
      |      |      |      |
      v      v      v      v
     Disk  Inode  Bitmap  Round-Robin
   Manager Table  /Directory
      |
      v
 virtual_disk.bin
```

## 3. Disk subsystem

### DiskManager

Responsibilities:
- Initialize the binary disk file.
- Read a fixed-size block.
- Write a fixed-size block.
- Reject invalid block numbers.

The default block size is 4096 bytes, and the default disk contains
1024 blocks.

The default disk capacity is therefore:

1024 x 4096 = 4,194,304 bytes (4 MiB).

### Superblock

The superblock describes filesystem geometry and metadata locations,
including block size, total blocks, inode counts, and filesystem region
offsets.

### Bitmap

A bitmap tracks whether resources are allocated. Separate bitmaps are
used for inode allocation and block allocation.

### Inode

An inode represents a file or directory and stores metadata such as
its type, size, permissions, timestamps, and data-block references.

### Directory

A directory maps a name to an inode number. The current implementation
uses an in-memory collection of directory entries.

## 4. File operations

The intended file-operation flow is:

```text
CLI command
    |
    v
FileSystem
    |
    +--> Resolve pathname
    |
    +--> Allocate or locate inode
    |
    +--> Allocate data block if needed
    |
    +--> Read/write through DiskManager
    |
    +--> Update filesystem metadata
```

The current prototype supports basic operations for entries directly
under the root directory. It does not yet implement a full nested
directory hierarchy.

## 5. CPU virtualization

### Process

A process stores:
- Process ID
- Name
- CPU burst duration
- Remaining execution time
- Priority
- Process state

### Scheduler

The scheduler implements Round-Robin time slicing.

Each ready process receives up to one time quantum. If it still has
remaining execution time, it returns to the end of the ready queue.

Example:

```text
P1 (5 units)
P2 (3 units)
Time quantum = 2

Execution order:
P1 -> P2 -> P1 -> P2 -> P1
```

This models fair sharing of a simulated CPU between processes.

## 6. Memory virtualization

### MemoryManager

The memory manager maintains simulated physical memory and tracks
available physical frames.

### PageTable

Each process has a page table mapping virtual pages to physical frames.

Address translation:

```text
Virtual address
       |
       +--> Virtual page number
       |
       +--> Offset within page
                 |
                 v
              PageTable
                 |
                 v
           Physical frame
                 |
                 v
Physical address = frame * page_size + offset
```

For a page size of 4096 bytes:

```text
Virtual address = 4200
Virtual page    = 4200 / 4096 = 1
Offset          = 4200 % 4096 = 104
```

If virtual page 1 maps to physical frame 3:

```text
Physical address = 3 * 4096 + 104
                 = 12392
```

The implementation currently allocates physical frames eagerly;
demand paging and page replacement are future extensions.

## 7. Testing strategy

| Test | Responsibility |
|---|---|
| test_disk | Disk initialization and block I/O |
| test_filesystem | File and directory operations |
| test_scheduler | Round-Robin execution order |
| test_memory | Address translation and frame management |

Tests are built as separate executables and can be run using CTest.

## 8. Current limitations

- Filesystem metadata is not fully persisted to the virtual disk.
- Formatting resets the disk in the current CLI.
- Directories are not a complete hierarchical filesystem.
- File data storage is limited by the current implementation.
- CPU scheduling and memory management are simulations, not kernel services.

The next architectural milestone is persistent filesystem metadata,
followed by nested paths, multi-block files, and recovery tests.

