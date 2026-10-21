cmake_minimum_required(VERSION 3.16)

project(vfs_gui
    VERSION 1.0.0
    DESCRIPTION "Qt C++ GUI for VirtualFileSystem and Operating System Simulator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Discover Qt6 from Homebrew or standard locations
list(PREPEND CMAKE_PREFIX_PATH
    "/opt/homebrew/opt/qtbase"
    "/opt/homebrew/opt/qt"
    "/usr/local/opt/qtbase"
    "/usr/local/opt/qt"
)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(GUI_SOURCES
    main.cpp
    mainwindow.cpp
    filesystem_tab.cpp
    scheduler_tab.cpp
    memory_tab.cpp
    terminal_tab.cpp
    gantt_widget.cpp
    frame_map_widget.cpp
)

set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/disk_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/superblock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/inode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/directory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/memory_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/page_table.cpp
)

add_executable(vfs_gui
    ${GUI_SOURCES}
    ${CORE_SOURCES}
)

target_include_directories(vfs_gui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(vfs_gui
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
