
#include "filesystem.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

int main() {
    const std::string diskPath = "data/test_filesystem.bin";

    {
        FileSystem fs(diskPath);

        assert(fs.format(128, 32));
        std::cout << "[PASS] Filesystem formatted\n";

        assert(fs.createFile("/hello.txt"));
        assert(fs.exists("/hello.txt"));

        std::cout << "[PASS] File created\n";

        const std::string message = "Hello, VirtualFileSystem!";

        assert(fs.writeFile("/hello.txt", message));
        assert(fs.readFile("/hello.txt") == message);

        std::cout << "[PASS] File write/read\n";

        assert(!fs.createFile("/hello.txt"));
        std::cout << "[PASS] Duplicate filename rejected\n";

        assert(fs.createDirectory("/documents"));
        assert(fs.exists("/documents"));

        std::cout << "[PASS] Directory created\n";

        assert(fs.deleteFile("/hello.txt"));
        assert(!fs.exists("/hello.txt"));

        std::cout << "[PASS] File deleted\n";

        assert(fs.deleteDirectory("/documents"));
        assert(!fs.exists("/documents"));

        std::cout << "[PASS] Directory deleted\n";

        assert(fs.unmount());
    }

    std::remove(diskPath.c_str());

    std::cout << "All filesystem tests passed.\n";
    return 0;
}

