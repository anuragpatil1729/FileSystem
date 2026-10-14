#ifndef PAGE_TABLE_HPP
#define PAGE_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

struct PageTableEntry {
    int frameNumber;
    bool present;
    bool writable;

    PageTableEntry(
        int frame = -1,
        bool isPresent = false,
        bool isWritable = true
    );
};

class PageTable {
public:
    explicit PageTable(
        std::uint32_t pid = 0
    );

    bool mapPage(
        std::size_t virtualPage,
        std::size_t physicalFrame
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

private:
    std::uint32_t pid_;

    std::vector<PageTableEntry> entries_;
};

#endif
