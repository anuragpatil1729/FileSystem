#ifndef DISK_MANAGER_HPP
#define DISK_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

/*
 * DiskManager exposes a host file as an array of fixed-size blocks.
 *
 * The block count is the *active geometry*: every read/write is bounds
 * checked against it. The geometry is set either when the image is
 * created (initialize) or after the real file size has been validated
 * (attachExisting). It never silently grows the image.
 */
class DiskManager {
public:
    static constexpr std::size_t BLOCK_SIZE = 4096;
    static constexpr std::size_t DEFAULT_BLOCK_COUNT = 1024;

    DiskManager(const std::string& diskPath,
                std::size_t blockCount = DEFAULT_BLOCK_COUNT);

    ~DiskManager();

    // Creates (or truncates) the image with the current block count.
    bool initialize();

    // Creates (or truncates) the image with a new geometry and adopts it.
    bool initialize(std::size_t blockCount);

    // Validates that the image exists and is a whole number of blocks,
    // then adopts the geometry derived from the real file size.
    bool attachExisting(std::string* error = nullptr);

    bool readBlock(std::size_t blockNumber, void* buffer) const;

    bool writeBlock(std::size_t blockNumber, const void* buffer);

    std::size_t getBlockCount() const;

    const std::string& getDiskPath() const;

    bool imageExists() const;

    // Returns false if the size cannot be determined.
    bool queryImageSize(std::uint64_t& bytes) const;

    /*
     * Test hook: after `successfulWrites` further successful writes,
     * the next `failureCount` writes fail without touching the image.
     * Used to exercise rollback paths deterministically.
     */
    void injectWriteFaults(std::size_t successfulWrites,
                           std::size_t failureCount);
    void clearWriteFaults();

private:
    std::string diskPath_;
    std::size_t blockCount_;

    std::size_t faultCountdown_;
    std::size_t faultFailures_;
};

#endif
