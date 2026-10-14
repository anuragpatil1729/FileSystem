#include "disk_manager.hpp"

#include <fstream>
#include <limits>
#include <vector>

namespace {
bool blockOffset(std::size_t blockNumber, std::streamoff& offset) {
    using Limits = std::numeric_limits<std::streamoff>;
    if (blockNumber > static_cast<std::size_t>(Limits::max()) /
                          DiskManager::BLOCK_SIZE) {
        return false;
    }
    offset = static_cast<std::streamoff>(blockNumber * DiskManager::BLOCK_SIZE);
    return true;
}
} // namespace

DiskManager::DiskManager(const std::string& diskPath,
                         std::size_t blockCount)
    : diskPath_(diskPath),
      blockCount_(blockCount),
      faultCountdown_(0),
      faultFailures_(0) {
}

DiskManager::~DiskManager() = default;

bool DiskManager::initialize() {
    return initialize(blockCount_);
}

bool DiskManager::initialize(std::size_t blockCount) {
    if (blockCount == 0) {
        return false;
    }

    std::streamoff ignored = 0;
    if (!blockOffset(blockCount, ignored)) {
        return false;
    }

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

    for (std::size_t i = 0; i < blockCount; ++i) {
        disk.write(emptyBlock.data(), BLOCK_SIZE);

        if (!disk) {
            return false;
        }
    }

    disk.flush();
    if (!disk) {
        return false;
    }

    blockCount_ = blockCount;
    return true;
}

bool DiskManager::imageExists() const {
    std::ifstream disk(diskPath_, std::ios::binary);
    return static_cast<bool>(disk);
}

bool DiskManager::queryImageSize(std::uint64_t& bytes) const {
    std::ifstream disk(diskPath_, std::ios::binary | std::ios::ate);
    if (!disk) {
        return false;
    }
    const std::streamoff end = disk.tellg();
    if (end < 0) {
        return false;
    }
    bytes = static_cast<std::uint64_t>(end);
    return true;
}

bool DiskManager::attachExisting(std::string* error) {
    auto fail = [error](const std::string& message) {
        if (error) *error = message;
        return false;
    };

    std::uint64_t bytes = 0;
    if (!queryImageSize(bytes)) {
        return fail("disk image '" + diskPath_ + "' does not exist or cannot be opened");
    }
    if (bytes < BLOCK_SIZE) {
        return fail("disk image is smaller than one block");
    }
    if (bytes % BLOCK_SIZE != 0) {
        return fail("disk image size is not a multiple of the block size");
    }
    const std::uint64_t blocks = bytes / BLOCK_SIZE;
    if (blocks > std::numeric_limits<std::size_t>::max()) {
        return fail("disk image is too large");
    }
    blockCount_ = static_cast<std::size_t>(blocks);
    return true;
}

bool DiskManager::readBlock(std::size_t blockNumber,
                            void* buffer) const {
    if (blockNumber >= blockCount_ || buffer == nullptr) {
        return false;
    }

    std::streamoff offset = 0;
    if (!blockOffset(blockNumber, offset)) {
        return false;
    }

    std::ifstream disk(
        diskPath_,
        std::ios::binary
    );

    if (!disk) {
        return false;
    }

    disk.seekg(offset);

    if (!disk) {
        return false;
    }

    disk.read(
        static_cast<char*>(buffer),
        BLOCK_SIZE
    );

    // A short read means the image is truncated: treat it as an error.
    return disk.gcount() == static_cast<std::streamsize>(BLOCK_SIZE);
}

bool DiskManager::writeBlock(
    std::size_t blockNumber,
    const void* buffer
) {
    if (blockNumber >= blockCount_ || buffer == nullptr) {
        return false;
    }

    if (faultFailures_ > 0) {
        if (faultCountdown_ == 0) {
            --faultFailures_;
            return false;
        }
        --faultCountdown_;
    }

    std::streamoff offset = 0;
    if (!blockOffset(blockNumber, offset)) {
        return false;
    }

    // Opening with in|out never creates or truncates the image, so a
    // missing image is reported instead of being silently recreated.
    std::fstream disk(
        diskPath_,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );

    if (!disk) {
        return false;
    }

    disk.seekp(offset);

    if (!disk) {
        return false;
    }

    disk.write(
        static_cast<const char*>(buffer),
        BLOCK_SIZE
    );

    disk.flush();
    return disk.good();
}

std::size_t DiskManager::getBlockCount() const {
    return blockCount_;
}

const std::string& DiskManager::getDiskPath() const {
    return diskPath_;
}

void DiskManager::injectWriteFaults(std::size_t successfulWrites,
                                    std::size_t failureCount) {
    faultCountdown_ = successfulWrites;
    faultFailures_ = failureCount;
}

void DiskManager::clearWriteFaults() {
    faultCountdown_ = 0;
    faultFailures_ = 0;
}
