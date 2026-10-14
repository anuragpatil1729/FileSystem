#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "disk_manager.hpp"
#include "superblock.hpp"
#include "inode.hpp"
#include "bitmap.hpp"
#include "directory.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class FileSystem {
public:
    static constexpr std::uint32_t DEFAULT_INODE_COUNT = 128;

    explicit FileSystem(const std::string& diskPath);

    bool format(std::size_t blockCount = DiskManager::DEFAULT_BLOCK_COUNT,
                std::uint32_t inodeCount = DEFAULT_INODE_COUNT);
    bool mount();
    bool unmount();

    bool createFile(const std::string& path);
    bool createDirectory(const std::string& path);
    bool writeFile(const std::string& path, const std::string& data);
    std::string readFile(const std::string& path);
    bool deleteFile(const std::string& path);
    bool deleteDirectory(const std::string& path);
    std::vector<std::string> listDirectory(const std::string& path = "/");
    bool exists(const std::string& path) const;
    void printInfo() const;

private:
    DiskManager disk_;
    Superblock superblock_;
    Bitmap inodeBitmap_;
    Bitmap blockBitmap_;
    std::vector<Inode> inodes_;
    Directory rootDirectory_;
    bool mounted_;

    int findInode(const std::string& path) const;
    int allocateInode(InodeType type);
    int allocateBlock();
    bool freeBlock(std::size_t blockNumber);
    std::string normalizePath(const std::string& path) const;

    bool persistMetadata();
    bool loadMetadata();
    bool persistRootDirectory();
    bool loadRootDirectory();
};

#endif

