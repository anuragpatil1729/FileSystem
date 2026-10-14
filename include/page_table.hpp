#ifndef PAGE_TABLE_HPP
#define PAGE_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

struct PageTableEntry {
    int frameNumber;
    bool present;
    bool writable;
    bool dirty;
    std::size_t lastAccessTime;

    PageTableEntry(
        int frame = -1,
        bool isPresent = false,
        bool isWritable = true
    );
};

class PageTable {
public:
    explicit PageTable(
        std::uint32_t pid = 0,
        std::size_t virtualMemorySize = 0
    );

    bool mapPage(
        std::size_t virtualPage,
        std::size_t physicalFrame,
        bool writable = true
    );

    bool unmapPage(
        std::size_t virtualPage
    );

    int getFrame(
        std::size_t virtualPage
    ) const;

    bool isMapped(
        std::size_t virtualPage
    ) const;

    std::uint32_t getPid() const;

    std::size_t size() const;

    std::size_t getVirtualMemorySize() const;
    void setVirtualMemorySize(std::size_t size);

    PageTableEntry* getEntry(std::size_t virtualPage);
    const PageTableEntry* getEntry(std::size_t virtualPage) const;

private:
    std::uint32_t pid_;
    std::size_t virtualMemorySize_;
    std::vector<PageTableEntry> entries_;
};

#endif
