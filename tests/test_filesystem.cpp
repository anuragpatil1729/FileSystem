#include "filesystem.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define TEST_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

namespace {

void cleanupFile(const std::string& path) {
    std::remove(path.c_str());
}

} // namespace

int main() {
    std::cout << "Starting comprehensive VirtualFileSystem test suite...\n";

    // 1. Basic format and operations (matching existing baseline test)
    {
        const std::string diskPath = "data/test_filesystem_basic.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        TEST_CHECK(fs.format(128, 32));
        std::cout << "[PASS] Filesystem formatted\n";

        TEST_CHECK(fs.createFile("/hello.txt"));
        TEST_CHECK(fs.exists("/hello.txt"));
        std::cout << "[PASS] File created\n";

        const std::string message = "Hello, VirtualFileSystem!";
        TEST_CHECK(fs.writeFile("/hello.txt", message));
        TEST_CHECK(fs.readFile("/hello.txt") == message);
        std::cout << "[PASS] File write/read\n";

        TEST_CHECK(!fs.createFile("/hello.txt"));
        std::cout << "[PASS] Duplicate filename rejected\n";

        TEST_CHECK(fs.createDirectory("/documents"));
        TEST_CHECK(fs.exists("/documents"));
        std::cout << "[PASS] Directory created\n";

        TEST_CHECK(fs.deleteFile("/hello.txt"));
        TEST_CHECK(!fs.exists("/hello.txt"));
        std::cout << "[PASS] File deleted\n";

        TEST_CHECK(fs.deleteDirectory("/documents"));
        TEST_CHECK(!fs.exists("/documents"));
        std::cout << "[PASS] Directory deleted\n";

        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    // 2. Hierarchical Directories & Path Traversal (from requirements)
    {
        const std::string diskPath = "data/test_filesystem_hierarchy.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        TEST_CHECK(fs.format(256, 64));

        // mkdir /projects
        TEST_CHECK(fs.createDirectory("/projects"));
        // mkdir /projects/VirtualOS
        TEST_CHECK(fs.createDirectory("/projects/VirtualOS"));
        // mkdir /projects/VirtualOS/src
        TEST_CHECK(fs.createDirectory("/projects/VirtualOS/src"));
        // create /projects/VirtualOS/README.md
        TEST_CHECK(fs.createFile("/projects/VirtualOS/README.md"));
        // write /projects/VirtualOS/README.md Initial documentation
        const std::string doc = "Initial documentation";
        TEST_CHECK(fs.writeFile("/projects/VirtualOS/README.md", doc));
        // read /projects/VirtualOS/README.md
        TEST_CHECK(fs.readFile("/projects/VirtualOS/README.md") == doc);

        // ls /
        const auto rootEntries = fs.listDirectory("/");
        TEST_CHECK(rootEntries.size() == 1 && rootEntries[0] == "projects");

        // ls /projects
        const auto projectsEntries = fs.listDirectory("/projects");
        TEST_CHECK(projectsEntries.size() == 1 && projectsEntries[0] == "VirtualOS");

        // ls /projects/VirtualOS
        const auto vOS = fs.listDirectory("/projects/VirtualOS");
        TEST_CHECK(vOS.size() == 2);

        // Path normalization tests: repeated slashes, trailing slashes
        TEST_CHECK(fs.exists("///projects///VirtualOS//src//"));
        TEST_CHECK(fs.readFile("///projects/VirtualOS//README.md") == doc);

        // Non-empty directory deletion refused
        TEST_CHECK(!fs.deleteDirectory("/projects/VirtualOS"));
        TEST_CHECK(!fs.deleteDirectory("/"));

        // rm /projects/VirtualOS/README.md
        TEST_CHECK(fs.deleteFile("/projects/VirtualOS/README.md"));
        TEST_CHECK(!fs.exists("/projects/VirtualOS/README.md"));

        // rmdir /projects/VirtualOS/src
        TEST_CHECK(fs.deleteDirectory("/projects/VirtualOS/src"));
        TEST_CHECK(!fs.exists("/projects/VirtualOS/src"));

        TEST_CHECK(fs.deleteDirectory("/projects/VirtualOS"));
        TEST_CHECK(fs.deleteDirectory("/projects"));

        std::cout << "[PASS] Hierarchical directory operations and path traversal\n";
        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    // 3. Multi-Block File Operations (Direct + Indirect blocks) & Resource Reclamation
    {
        const std::string diskPath = "data/test_filesystem_multiblock.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        TEST_CHECK(fs.format(512, 64));

        const std::size_t initialFreeBlocks = fs.getSuperblock().freeBlocks;
        const std::size_t initialFreeInodes = fs.getSuperblock().freeInodes;

        // Empty file
        TEST_CHECK(fs.createFile("/empty.txt"));
        TEST_CHECK(fs.exists("/empty.txt"));
        TEST_CHECK(fs.readFile("/empty.txt").empty());

        // File spanning 3 blocks (10,000 bytes)
        std::string data10k(10000, 'A');
        for (std::size_t i = 0; i < data10k.size(); ++i) {
            data10k[i] = static_cast<char>('A' + (i % 26));
        }
        TEST_CHECK(fs.createFile("/multi3.bin"));
        TEST_CHECK(fs.writeFile("/multi3.bin", data10k));
        TEST_CHECK(fs.readFile("/multi3.bin") == data10k);

        // File spanning into indirect block (14 blocks = 14 * 4096 = 57344 bytes)
        std::string dataIndirect(57344, 'Z');
        for (std::size_t i = 0; i < dataIndirect.size(); ++i) {
            dataIndirect[i] = static_cast<char>('0' + (i % 10));
        }
        TEST_CHECK(fs.createFile("/indirect.bin"));
        TEST_CHECK(fs.writeFile("/indirect.bin", dataIndirect));
        TEST_CHECK(fs.readFile("/indirect.bin") == dataIndirect);

        // Overwrite and truncate
        std::string small("Short content");
        TEST_CHECK(fs.writeFile("/indirect.bin", small));
        TEST_CHECK(fs.readFile("/indirect.bin") == small);

        // Delete all files and verify complete block and inode reclamation
        TEST_CHECK(fs.deleteFile("/empty.txt"));
        TEST_CHECK(fs.deleteFile("/multi3.bin"));
        TEST_CHECK(fs.deleteFile("/indirect.bin"));

        TEST_CHECK(fs.getSuperblock().freeBlocks == initialFreeBlocks);
        TEST_CHECK(fs.getSuperblock().freeInodes == initialFreeInodes);

        std::cout << "[PASS] Multi-block files, indirect blocks, and resource reclamation\n";
        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    // 4. Persistence across Destruction and Reconstruction
    {
        const std::string diskPath = "data/test_filesystem_persist.bin";
        cleanupFile(diskPath);

        const std::string content1 = "First file data spanning multiple characters";
        const std::string content2(9000, 'X');

        {
            FileSystem fs(diskPath);
            TEST_CHECK(fs.format(256, 32));
            TEST_CHECK(fs.createDirectory("/dir1"));
            TEST_CHECK(fs.createDirectory("/dir1/dir2"));
            TEST_CHECK(fs.createFile("/dir1/file1.txt"));
            TEST_CHECK(fs.writeFile("/dir1/file1.txt", content1));
            TEST_CHECK(fs.createFile("/dir1/dir2/file2.bin"));
            TEST_CHECK(fs.writeFile("/dir1/dir2/file2.bin", content2));
            TEST_CHECK(fs.unmount());
        }

        // Reconstruct new FileSystem instance without formatting
        {
            FileSystem fs(diskPath);
            TEST_CHECK(fs.mount());
            TEST_CHECK(fs.exists("/dir1/file1.txt"));
            TEST_CHECK(fs.readFile("/dir1/file1.txt") == content1);
            TEST_CHECK(fs.exists("/dir1/dir2/file2.bin"));
            TEST_CHECK(fs.readFile("/dir1/dir2/file2.bin") == content2);
            TEST_CHECK(fs.unmount());
        }

        std::cout << "[PASS] Persistence across destruction and reconstruction\n";
        cleanupFile(diskPath);
    }

    // 5. Inode and Block Exhaustion
    {
        const std::string diskPath = "data/test_filesystem_exhaustion.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        // Small filesystem: 32 blocks, 8 inodes (1 root, leaves 7 free inodes)
        TEST_CHECK(fs.format(32, 8));

        int created = 0;
        for (int i = 0; i < 10; ++i) {
            if (fs.createFile("/f" + std::to_string(i))) {
                ++created;
            }
        }
        TEST_CHECK(created == 7); // Exactly 7 user inodes allowed
        TEST_CHECK(!fs.createFile("/overflow.txt"));

        // Reclaim an inode and create again
        TEST_CHECK(fs.deleteFile("/f0"));
        TEST_CHECK(fs.createFile("/new_file.txt"));

        std::cout << "[PASS] Inode exhaustion handled safely\n";
        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    // 6. Corrupted Metadata Detection
    {
        const std::string diskPath = "data/test_filesystem_corrupt.bin";
        cleanupFile(diskPath);

        {
            FileSystem fs(diskPath);
            TEST_CHECK(fs.format(64, 16));
            TEST_CHECK(fs.unmount());
        }

        // Corrupt superblock magic
        {
            std::fstream f(diskPath, std::ios::in | std::ios::out | std::ios::binary);
            char zero[4] = {0, 0, 0, 0};
            f.seekp(0);
            f.write(zero, 4);
        }

        {
            FileSystem fs(diskPath);
            TEST_CHECK(!fs.mount()); // Must reject bad magic
        }

        std::cout << "[PASS] Corrupted superblock safely rejected\n";
        cleanupFile(diskPath);
    }

    // 7. Maximum file size rejection
    {
        const std::string diskPath = "data/test_filesystem_maxsize.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        TEST_CHECK(fs.format(2048, 64));
        TEST_CHECK(fs.createFile("/large.bin"));

        // Max file size is Inode::MAX_FILE_SIZE. Anything larger must be rejected.
        std::string huge(Inode::MAX_FILE_SIZE + 10, 'H');
        TEST_CHECK(!fs.writeFile("/large.bin", huge));

        std::cout << "[PASS] File size limit enforced\n";
        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    // 8. Rollback on write failure
    {
        const std::string diskPath = "data/test_filesystem_fault.bin";
        cleanupFile(diskPath);

        FileSystem fs(diskPath);
        TEST_CHECK(fs.format(128, 32));
        TEST_CHECK(fs.createFile("/target.txt"));

        const std::size_t freeBefore = fs.getSuperblock().freeBlocks;

        // Inject disk failure after 1 block write
        fs.getDiskManager().injectWriteFaults(1, 1);
        std::string multiBlockData(8192, 'D');
        TEST_CHECK(!fs.writeFile("/target.txt", multiBlockData));
        fs.getDiskManager().clearWriteFaults();

        // Verify allocated blocks were rolled back
        TEST_CHECK(fs.getSuperblock().freeBlocks == freeBefore);

        std::cout << "[PASS] Staged allocation rollback on write failure\n";
        TEST_CHECK(fs.unmount());
        cleanupFile(diskPath);
    }

    std::cout << "\nAll comprehensive filesystem tests passed successfully!\n";
    return 0;
}
