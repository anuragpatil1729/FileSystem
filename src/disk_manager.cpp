#include "disk_manager.hpp"

#include <fstream>
#include <vector>

DiskManager::DiskManager(const std::string& diskPath,
                         std::size_t blockCount)
    : diskPath_(diskPath),
      blockCount_(blockCount) {
}

DiskManager::~DiskManager() = default;

bool DiskManager::initialize() {
    std::ofstream disk(
        diskPath_,
        std::ios::binary |
        std::ios::out |
        std::ios::trunc
    );

    if (!disk) {
        return false;
    }

    std::vector<char> emptyBlock(BLOCK_SIZE, 0);

    for (std::size_t i = 0; i < blockCount_; ++i) {
        disk.write(emptyBlock.data(), BLOCK_SIZE);

        if (!disk) {
            return false;
        }
    }

    return true;
}

bool DiskManager::readBlock(std::size_t blockNumber,
                            void* buffer) {
    if (blockNumber >= blockCount_ || buffer == nullptr) {
        return false;
    }

    std::ifstream disk(
        diskPath_,
        std::ios::binary
    );

    if (!disk) {
        return false;
    }

    const std::streamoff offset =
        static_cast<std::streamoff>(
            blockNumber * BLOCK_SIZE
        );

    disk.seekg(offset);

    if (!disk) {
        return false;
    }

    disk.read(
        static_cast<char*>(buffer),
        BLOCK_SIZE
    );

    return disk.good() || disk.gcount() == BLOCK_SIZE;
}

bool DiskManager::writeBlock(
    std::size_t blockNumber,
    const void* buffer
) {
    if (blockNumber >= blockCount_ || buffer == nullptr) {
        return false;
    }

    std::fstream disk(
        diskPath_,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );

    if (!disk) {
        return false;
    }

    const std::streamoff offset =
        static_cast<std::streamoff>(
            blockNumber * BLOCK_SIZE
        );

    disk.seekp(offset);

    if (!disk) {
        return false;
    }

    disk.write(
        static_cast<const char*>(buffer),
        BLOCK_SIZE
    );

    return disk.good();
}

std::size_t DiskManager::getBlockCount() const {
    return blockCount_;
}

const std::string& DiskManager::getDiskPath() const {
    return diskPath_;
}
