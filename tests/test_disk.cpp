#include "disk_manager.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#define TEST_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main() {
    const std::string path = "data/test_disk.bin";
    std::remove(path.c_str());

    // 1. Basic initialization & block read/write
    {
        DiskManager disk(path, 8);
        TEST_CHECK(disk.initialize());
        std::cout << "[PASS] Disk initialized\n";

        std::array<char, DiskManager::BLOCK_SIZE> written{};
        std::array<char, DiskManager::BLOCK_SIZE> read{};

        written[0] = 'V';
        written[1] = 'F';
        written[2] = 'S';

        TEST_CHECK(disk.writeBlock(2, written.data()));
        TEST_CHECK(disk.readBlock(2, read.data()));

        TEST_CHECK(read[0] == 'V');
        TEST_CHECK(read[1] == 'F');
        TEST_CHECK(read[2] == 'S');
        std::cout << "[PASS] Block read/write\n";

        TEST_CHECK(!disk.readBlock(8, read.data()));
        TEST_CHECK(!disk.writeBlock(8, written.data()));
        std::cout << "[PASS] Invalid block rejected\n";
    }

    // 2. Dynamic geometry resize
    {
        DiskManager disk(path, 8);
        TEST_CHECK(disk.initialize(16));
        TEST_CHECK(disk.getBlockCount() == 16);

        std::array<char, DiskManager::BLOCK_SIZE> block{};
        block[0] = 'K';
        TEST_CHECK(disk.writeBlock(15, block.data()));
        TEST_CHECK(disk.readBlock(15, block.data()));
        TEST_CHECK(block[0] == 'K');
        std::cout << "[PASS] Disk geometry update\n";
    }

    // 3. Attach existing disk and validate geometry from file size
    {
        DiskManager disk(path, 0);
        std::string err;
        TEST_CHECK(disk.attachExisting(&err));
        TEST_CHECK(disk.getBlockCount() == 16);
        std::cout << "[PASS] Attach existing disk image\n";
    }

    // 4. Reject truncated or non-multiple file sizes
    {
        const std::string badPath = "data/test_disk_bad.bin";
        std::remove(badPath.c_str());
        {
            std::ofstream f(badPath, std::ios::binary);
            f.write("small", 5);
        }
        DiskManager badDisk(badPath, 0);
        std::string err;
        TEST_CHECK(!badDisk.attachExisting(&err));
        std::remove(badPath.c_str());
        std::cout << "[PASS] Reject truncated disk image\n";
    }

    // 5. Fault injection verification
    {
        DiskManager disk(path, 16);
        std::array<char, DiskManager::BLOCK_SIZE> buf{};
        disk.injectWriteFaults(1, 2);
        TEST_CHECK(disk.writeBlock(0, buf.data()));  // 1st write succeeds
        TEST_CHECK(!disk.writeBlock(0, buf.data())); // 2nd write injected failure
        TEST_CHECK(!disk.writeBlock(0, buf.data())); // 3rd write injected failure
        TEST_CHECK(disk.writeBlock(0, buf.data()));  // 4th write succeeds
        disk.clearWriteFaults();
        std::cout << "[PASS] Disk fault injection\n";
    }

    std::remove(path.c_str());
    std::cout << "All disk tests passed.\n";
    return 0;
}
