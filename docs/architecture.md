# VirtualFileSystem Architecture

## 1. Overview

VirtualFileSystem is a functional C++17 user-space operating system environment featuring a persistent block-based filesystem, a Round-Robin CPU process scheduler with detailed performance metrics, and a virtual memory manager supporting demand paging and page replacement.

---

## 2. System Architecture

```text
                               CLI (src/main.cpp)
                                       |
             +-------------------------+-------------------------+
             |                         |                         |
             v                         v                         v
        FileSystem                 Scheduler               MemoryManager
             |                         |                         |
     +-------+-------+             Process                PageTable / Frames
     |       |       |                 |                         |
     v       v       v                 v                         v
 Superblock Inode  Bitmap         Round-Robin             FIFO / LRU
   (v2)    Table  Allocators      Timeline & Metrics      Demand Paging
     |       |       |
     +-------+-------+
             |
             v
        DiskManager
             |
             v
      virtual_disk.bin
```

---

## 3. On-Disk Filesystem Architecture

### 3.1 Disk Layout (Version 2)

The virtual disk is divided into fixed 4096-byte blocks:

```text
+-------------------+--------------------+--------------------+--------------------+--------------------+
| Block 0           | Block 1 .. S1      | Block S1+1 .. S2   | Block S2+1 .. S3   | Block S3+1 .. End  |
| Superblock        | Inode Bitmap       | Block Bitmap       | Inode Table        | Data Blocks        |
+-------------------+--------------------+--------------------+--------------------+--------------------+
```

The region boundaries are computed as a deterministic function of `(totalBlocks, totalInodes)`:
- `inodeBitmapBlocks` = $\lceil \text{totalInodes} / (4096 \times 8) \rceil$
- `blockBitmapBlocks` = $\lceil \text{totalBlocks} / (4096 \times 8) \rceil$
- `inodeTableBlocks`  = $\lceil \text{totalInodes} / 32 \rceil$ (since each on-disk inode record is 128 bytes)
- `dataBlockStart`    = $1 + \text{inodeBitmapBlocks} + \text{blockBitmapBlocks} + \text{inodeTableBlocks}$

### 3.2 Superblock
Stores global metadata:
- `magic`: `0x56534653` ("VSFS").
- `version`: `2`.
- `blockSize`: `4096`.
- `totalBlocks`, `freeBlocks`, `totalInodes`, `freeInodes`.
- Region start block numbers and block counts.
- `rootInode`: `0`.
- Little-endian serialized fields via `byte_io.hpp`.

### 3.3 Inodes & Multi-Block Storage
Each inode is serialized to an exact 128-byte record (32 records per 4096-byte block):
- `inodeNumber` (4 bytes)
- `type` (1 byte: `FREE=0`, `FILE=1`, `DIRECTORY=2`)
- `size` (4 bytes)
- `owner`, `permissions`, `linkCount` (4 bytes each)
- `directBlocks[12]` (48 bytes): direct pointers to data blocks 0..11.
- `indirectBlock` (4 bytes): pointer to a single indirect block containing 1024 32-bit block pointers.
- `createdAt`, `modifiedAt` (8 bytes each, Unix timestamp in seconds).
- `reserved` (36 bytes zero-padding).

**Maximum Supported File Size**:
$$(12 \text{ direct blocks} + 1024 \text{ indirect pointers}) \times 4096 \text{ bytes} = 1036 \text{ blocks} = 4,243,456 \text{ bytes } (\approx 4.047 \text{ MiB})$$

### 3.4 Hierarchical Directories
Directories are stored as standard inodes with `InodeType::DIRECTORY`.
Directory data blocks hold an array of 128-byte entries:
- `inodeNumber` (4 bytes)
- `nameLength` (4 bytes)
- `name` (up to 120 bytes, null-terminated)

Path resolution recursively splits paths on `/`, normalizes redundant and trailing separators, and traverses the directory tree starting from root Inode 0.

### 3.5 Metadata Consistency & Integrity Audit
On mount, `FileSystem::validateMetadataConsistency()` verifies:
1. Superblock magic, version, block size, and boundary math.
2. Actual image file size on host matches `totalBlocks`.
3. Root inode exists, is marked allocated in `inodeBitmap`, and is a `DIRECTORY`.
4. All allocated inodes are marked in `inodeBitmap` and have valid types.
5. All data block pointers are within bounds $[\text{dataBlockStart}, \text{totalBlocks})$.
6. Block leak detection: constructs a reference-count map across all inodes (direct and indirect). Any orphan allocated block or double-allocated block aborts the mount.
7. Directory entry validity: entries reference allocated inodes with unique names.

---

## 4. CPU Virtualization & Scheduling

### 4.1 Process Model
Represents simulated processes:
- `pid`, `name`, `priority`, `arrivalTime`.
- `burstTime` and `remainingTime`.
- State transitions: `NEW` $\to$ `READY` $\to$ `RUNNING` $\to$ `TERMINATED` (or `BLOCKED`).

### 4.2 Round-Robin Algorithm
- Executes up to `timeQuantum` units per slice.
- Respects process arrival times and accounts for CPU idle intervals.
- Generates a chronological Gantt timeline with start and end timestamps.

### 4.3 Scheduling Metrics
Computes simulated execution metrics:
- **Completion Time** ($C_i$): Time unit when process finishes execution.
- **Turnaround Time** ($TAT_i = C_i - \text{Arrival}_i$).
- **Waiting Time** ($WT_i = TAT_i - \text{Burst}_i$).
- **Response Time** ($RT_i = \text{FirstSchedule}_i - \text{Arrival}_i$).
- **Average Waiting Time** & **Average Turnaround Time**.
- **CPU Utilization (%)** = $(\text{Total Busy Time} / \text{Total Elapsed Time}) \times 100$.

---

## 5. Memory Virtualization

### 5.1 Paging Model
- Physical memory is simulated as a byte buffer organized into 4096-byte frames.
- Frame allocation tracked via `frameBitmap_`.
- Each process has an isolated `PageTable`.

### 5.2 Address Translation & Bounds Checking
Virtual address translation maps $(\text{virtualPage}, \text{offset})$ to physical frame:
$$\text{Physical Address} = (\text{frameNumber} \times 4096) + \text{offset}$$
Safeguards:
- Rejects virtual addresses beyond the process's allocated virtual size.
- Detects integer overflows.
- Enforces write permissions.

### 5.3 Demand Paging & Page Replacement
- **Eager Allocation**: Frames allocated up-front during process creation.
- **Demand Paging**: Virtual pages are created with `present = false`. Access triggers a page fault:
  - If a free frame exists, it is allocated and mapped.
  - If physical memory is full, the configured page replacement algorithm evicts a victim frame:
    - **FIFO**: Evicts the frame allocated earliest.
    - **LRU**: Evicts the frame with the oldest access timestamp.
  - Eviction unmaps the page from the victim process table.
  - Metrics track total page faults and page replacements.

---

## 6. Testing Strategy

1. `test_disk`: Block I/O, dynamic geometry updating, `attachExisting` validation, and fault injection.
2. `test_filesystem`: Hierarchical directories, path normalization, multi-block files, indirect blocks, persistence across restarts, inode/block exhaustion, corrupt image rejection, and staged rollback.
3. `test_scheduler`: Round-robin execution, arrival times, idle handling, turnaround/waiting/response metrics, and repeatability.
4. `test_memory`: Address translation, eager allocation, bounds checking, demand paging, FIFO replacement, and LRU replacement.
