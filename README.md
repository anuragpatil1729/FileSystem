
# VirtualFileSystem

A C++17 educational project demonstrating filesystem internals and
selected operating-system concepts through a simulated disk, file
metadata, CPU scheduling, and virtual memory.

## Features

### Virtual filesystem
- Simulated disk backed by a binary file.
- Fixed-size disk blocks.
- Superblock metadata.
- Inode-based file metadata.
- Bitmap-based resource allocation.
- Directory entries mapping names to inode numbers.
- Basic file creation, reading, writing, listing, and deletion.

### CPU virtualization
- Simulated processes with process IDs.
- Process states and CPU burst tracking.
- Round-Robin scheduling with configurable time quantum.
- Execution-order logging.

### Memory virtualization
- Simulated physical memory.
- Fixed-size pages and frames.
- Per-process page tables.
- Virtual-to-physical address translation.
- Memory read/write operations and frame reclamation.

## Tech stack

- C++17
- CMake
- GNU Make
- Standard C++ library

## Project structure

```text
VirtualFileSystem/
├── include/       # Header files
├── src/           # Implementations and CLI
├── tests/         # Component tests
├── docs/          # Architecture documentation
├── data/          # Virtual disk files
├── CMakeLists.txt
├── Makefile
├── README.md
└── .gitignore
```

## Requirements

- A C++17-compatible compiler
- CMake 3.16 or newer for the CMake workflow
- GNU Make for the Makefile workflow

On macOS, install Apple's command-line developer tools if necessary:

```bash
xcode-select --install
```

Check your tools:

```bash
clang++ --version
cmake --version
make --version
```

## Build with CMake

Run these commands from the project root:

```bash
cmake -S . -B build
cmake --build build
```

Run the application:

```bash
./build/vfs
```

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

## Build with Make

Build the application:

```bash
make
```

Run the application:

```bash
make run
```

Run all component tests:

```bash
make test
```

Remove generated build files:

```bash
make clean
```

## CLI commands

Once the application starts, type `help` to see available commands.

Examples:

```text
create /hello.txt
write /hello.txt Hello World
read /hello.txt
ls
info
cpu
mem
rm /hello.txt
exit
```

## Current implementation scope

This is an educational prototype, not a production filesystem or
a complete operating system.

Current limitations include:

- Filesystem metadata is not yet fully serialized to disk.
- The current CLI formats the virtual disk on startup.
- Directory handling is flat rather than a complete nested hierarchy.
- File storage currently supports a limited data-block implementation.
- CPU scheduling and memory management are simulations within a
  user-space program, not kernel-level virtualization.
- Crash recovery, permissions enforcement, concurrency, and
  persistent mounting are not implemented.

## Future improvements

- Persist and reload superblocks, bitmaps, inodes, and directories.
- Implement nested directories and path traversal.
- Support files spanning multiple data blocks.
- Add filesystem consistency checks and recovery.
- Add process arrival times and scheduling metrics.
- Implement page replacement and page-fault handling.
- Expand tests for failure cases and persistence.

## License

Choose and add a license before distributing or accepting contributions.

