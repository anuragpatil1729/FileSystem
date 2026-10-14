
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude

BUILD_DIR = build
TARGET = $(BUILD_DIR)/vfs

CORE_SOURCES = \
	src/disk_manager.cpp \
	src/superblock.cpp \
	src/inode.cpp \
	src/bitmap.cpp \
	src/directory.cpp \
	src/filesystem.cpp \
	src/process.cpp \
	src/scheduler.cpp \
	src/memory_manager.cpp \
	src/page_table.cpp

TESTS = test_disk test_filesystem test_scheduler test_memory

.PHONY: all run test test_disk test_filesystem test_scheduler test_memory clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): src/main.cpp $(CORE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_disk: tests/test_disk.cpp $(CORE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_filesystem: tests/test_filesystem.cpp $(CORE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_scheduler: tests/test_scheduler.cpp $(CORE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_memory: tests/test_memory.cpp $(CORE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(TARGET)
	./$(TARGET)

test: $(addprefix $(BUILD_DIR)/,$(TESTS))
	./$(BUILD_DIR)/test_disk
	./$(BUILD_DIR)/test_filesystem
	./$(BUILD_DIR)/test_scheduler
	./$(BUILD_DIR)/test_memory

test_disk: $(BUILD_DIR)/test_disk
	./$(BUILD_DIR)/test_disk

test_filesystem: $(BUILD_DIR)/test_filesystem
	./$(BUILD_DIR)/test_filesystem

test_scheduler: $(BUILD_DIR)/test_scheduler
	./$(BUILD_DIR)/test_scheduler

test_memory: $(BUILD_DIR)/test_memory
	./$(BUILD_DIR)/test_memory

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o vfs test_disk test_filesystem test_scheduler test_memory

