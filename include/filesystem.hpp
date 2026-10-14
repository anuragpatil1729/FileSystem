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

    // Additional diagnostic and inspection methods
    bool isMounted() const;
    const std::string& getLastError() const;
    const Superblock& getSuperblock() const;
    DiskManager& getDiskManager();
    const DiskManager& getDiskManager() const;
    std::size_t getMaxFileSize() const;

private:
    DiskManager disk_;
    Superblock superblock_;
    Bitmap inodeBitmap_;
    Bitmap blockBitmap_;
    std::vector<Inode> inodes_;
    bool mounted_;
    mutable std::string lastError_;

    void setError(const std::string& msg) const;

    int findInode(const std::string& path) const;
    int allocateInode(InodeType type);
    int allocateBlock();
    bool freeBlock(std::size_t blockNumber);
    void freeInodeBlocks(Inode& inode);

    static std::vector<std::string> splitPath(const std::string& path);
    bool resolveParentAndName(const std::string& path,
                              std::uint32_t& parentInode,
                              std::string& entryName) const;
    int resolvePath(const std::string& path) const;

    bool loadDirectory(std::uint32_t inodeNumber, Directory& dir) const;
    bool saveDirectory(std::uint32_t inodeNumber, const Directory& dir);

    bool readInodeData(std::uint32_t inodeNumber, std::vector<std::uint8_t>& out) const;
    bool writeInodeData(std::uint32_t inodeNumber, const void* data, std::size_t size);

    bool persistMetadata();
    bool loadMetadata();
    bool validateMetadataConsistency(std::string* error) const;
};

#endif
