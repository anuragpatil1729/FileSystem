
cmake_minimum_required(VERSION 3.16)

project(VirtualFileSystem
    VERSION 1.0.0
    DESCRIPTION "Virtual filesystem with CPU and memory virtualization"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Shared implementation library.
add_library(vfs_core
    src/disk_manager.cpp
    src/superblock.cpp
    src/inode.cpp
    src/bitmap.cpp
    src/directory.cpp
    src/filesystem.cpp
    src/process.cpp
    src/scheduler.cpp
    src/memory_manager.cpp
    src/page_table.cpp
)

target_include_directories(vfs_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main application.
add_executable(vfs
    src/main.cpp
)

target_link_libraries(vfs PRIVATE vfs_core)

# Tests.
enable_testing()

add_executable(test_disk
    tests/test_disk.cpp
)
target_link_libraries(test_disk PRIVATE vfs_core)

add_executable(test_filesystem
    tests/test_filesystem.cpp
)
target_link_libraries(test_filesystem PRIVATE vfs_core)

add_executable(test_scheduler
    tests/test_scheduler.cpp
)
target_link_libraries(test_scheduler PRIVATE vfs_core)

add_executable(test_memory
    tests/test_memory.cpp
)
target_link_libraries(test_memory PRIVATE vfs_core)

add_test(NAME DiskTests COMMAND test_disk)
add_test(NAME FilesystemTests COMMAND test_filesystem)
add_test(NAME SchedulerTests COMMAND test_scheduler)
add_test(NAME MemoryTests COMMAND test_memory)

# Tests use paths relative to the project root.
set_tests_properties(
    DiskTests
    FilesystemTests
    SchedulerTests
    MemoryTests
    PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

