#ifndef DISK_MANAGER_HPP
#define DISK_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

class DiskManager {
public:
    static constexpr std::size_t BLOCK_SIZE = 4096;
    static constexpr std::size_t DEFAULT_BLOCK_COUNT = 1024;

    DiskManager(const std::string& diskPath,
                std::size_t blockCount = DEFAULT_BLOCK_COUNT);

    ~DiskManager();

    bool initialize();

    bool readBlock(std::size_t blockNumber, void* buffer);

    bool writeBlock(std::size_t blockNumber, const void* buffer);

    std::size_t getBlockCount() const;

    const std::string& getDiskPath() const;

private:
    std::string diskPath_;
    std::size_t blockCount_;
};

#endif
