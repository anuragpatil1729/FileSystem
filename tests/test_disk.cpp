
#include "disk_manager.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

int main() {
    const std::string path = "data/test_disk.bin";

    DiskManager disk(path, 8);

    assert(disk.initialize());
    std::cout << "[PASS] Disk initialized\n";

    std::array<char, DiskManager::BLOCK_SIZE> written{};
    std::array<char, DiskManager::BLOCK_SIZE> read{};

    written[0] = 'V';
    written[1] = 'F';
    written[2] = 'S';

    assert(disk.writeBlock(2, written.data()));
    assert(disk.readBlock(2, read.data()));

    assert(read[0] == 'V');
    assert(read[1] == 'F');
    assert(read[2] == 'S');

    std::cout << "[PASS] Block read/write\n";

    assert(!disk.readBlock(8, read.data()));
    assert(!disk.writeBlock(8, written.data()));

    std::cout << "[PASS] Invalid block rejected\n";

    std::remove(path.c_str());

    std::cout << "All disk tests passed.\n";
    return 0;
}

