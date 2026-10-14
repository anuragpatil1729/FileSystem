#include "page_table.hpp"

PageTableEntry::PageTableEntry(
    int frame,
    bool isPresent,
    bool isWritable
)
    : frameNumber(frame),
      present(isPresent),
      writable(isWritable),
      dirty(false),
      lastAccessTime(0) {
}

PageTable::PageTable(
    std::uint32_t pid,
    std::size_t virtualMemorySize
)
    : pid_(pid),
      virtualMemorySize_(virtualMemorySize) {
}

bool PageTable::mapPage(
    std::size_t virtualPage,
    std::size_t physicalFrame,
    bool writable
) {
    if (virtualPage >= entries_.size()) {
        entries_.resize(virtualPage + 1);
    }

    if (entries_[virtualPage].present) {
        return false;
    }

    entries_[virtualPage] = PageTableEntry(
        static_cast<int>(physicalFrame),
        true,
        writable
    );

    return true;
}

bool PageTable::unmapPage(
    std::size_t virtualPage
) {
    if (virtualPage >= entries_.size()) {
        return false;
    }

    if (!entries_[virtualPage].present) {
        return false;
    }

    entries_[virtualPage] = PageTableEntry();
    return true;
}

int PageTable::getFrame(
    std::size_t virtualPage
) const {
    if (virtualPage >= entries_.size()) {
        return -1;
    }

    if (!entries_[virtualPage].present) {
        return -1;
    }

    return entries_[virtualPage].frameNumber;
}

bool PageTable::isMapped(
    std::size_t virtualPage
) const {
    if (virtualPage >= entries_.size()) {
        return false;
    }

    return entries_[virtualPage].present;
}

std::uint32_t PageTable::getPid() const {
    return pid_;
}

std::size_t PageTable::size() const {
    return entries_.size();
}

std::size_t PageTable::getVirtualMemorySize() const {
    return virtualMemorySize_;
}

void PageTable::setVirtualMemorySize(std::size_t size) {
    virtualMemorySize_ = size;
}

PageTableEntry* PageTable::getEntry(std::size_t virtualPage) {
    if (virtualPage >= entries_.size()) {
        return nullptr;
    }
    return &entries_[virtualPage];
}

const PageTableEntry* PageTable::getEntry(std::size_t virtualPage) const {
    if (virtualPage >= entries_.size()) {
        return nullptr;
    }
    return &entries_[virtualPage];
}
