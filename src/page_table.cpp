#include "page_table.hpp"

PageTableEntry::PageTableEntry(
    int frame,
    bool isPresent,
    bool isWritable
)
    : frameNumber(frame),
      present(isPresent),
      writable(isWritable) {
}

PageTable::PageTable(
    std::uint32_t pid
)
    : pid_(pid) {
}

bool PageTable::mapPage(
    std::size_t virtualPage,
    std::size_t physicalFrame
) {
    /*
     * Expand the page table if necessary.
     */
    if (
        virtualPage >= entries_.size()
    ) {
        entries_.resize(
            virtualPage + 1
        );
    }

    /*
     * Don't overwrite an existing mapping.
     */
    if (
        entries_[virtualPage].present
    ) {
        return false;
    }

    entries_[virtualPage] =
        PageTableEntry(
            static_cast<int>(
                physicalFrame
            ),
            true,
            true
        );

    return true;
}

bool PageTable::unmapPage(
    std::size_t virtualPage
) {
    if (
        virtualPage >= entries_.size()
    ) {
        return false;
    }

    if (
        !entries_[virtualPage].present
    ) {
        return false;
    }

    entries_[virtualPage] =
        PageTableEntry();

    return true;
}

int PageTable::getFrame(
    std::size_t virtualPage
) const {
    if (
        virtualPage >= entries_.size()
    ) {
        return -1;
    }

    if (
        !entries_[virtualPage].present
    ) {
        return -1;
    }

    return entries_[virtualPage].frameNumber;
}

bool PageTable::isMapped(
    std::size_t virtualPage
) const {
    if (
        virtualPage >= entries_.size()
    ) {
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
