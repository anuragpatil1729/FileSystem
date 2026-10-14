#include "memory_manager.hpp"

#include <iostream>

MemoryManager::MemoryManager(
    std::size_t physicalMemorySize
)
    : physicalMemory_(
          physicalMemorySize,
          0
      ),
      frameBitmap_(
          physicalMemorySize / PAGE_SIZE,
          false
      ) {
}

int MemoryManager::allocateFrame() {
    for (
        std::size_t i = 0;
        i < frameBitmap_.size();
        ++i
    ) {
        if (!frameBitmap_[i]) {
            frameBitmap_[i] = true;

            return static_cast<int>(i);
        }
    }

    return -1;
}

void MemoryManager::freeFrame(
    std::size_t frameNumber
) {
    if (
        frameNumber >= frameBitmap_.size()
    ) {
        return;
    }

    frameBitmap_[frameNumber] = false;
}

PageTable* MemoryManager::findPageTable(
    std::uint32_t pid
) {
    for (auto& table : pageTables_) {
        if (table.getPid() == pid) {
            return &table;
        }
    }

    return nullptr;
}

const PageTable*
MemoryManager::findPageTable(
    std::uint32_t pid
) const {
    for (const auto& table : pageTables_) {
        if (table.getPid() == pid) {
            return &table;
        }
    }

    return nullptr;
}

bool MemoryManager::allocateProcess(
    std::uint32_t pid,
    std::size_t virtualMemorySize
) {
    if (virtualMemorySize == 0) {
        return false;
    }

    /*
     * Don't create duplicate address spaces.
     */
    if (findPageTable(pid) != nullptr) {
        return false;
    }

    /*
     * Calculate required virtual pages.
     */
    std::size_t pageCount =
        (virtualMemorySize + PAGE_SIZE - 1)
        / PAGE_SIZE;

    PageTable table(pid);

    /*
     * Allocate a physical frame for every
     * virtual page.
     */
    for (
        std::size_t page = 0;
        page < pageCount;
        ++page
    ) {
        int frame = allocateFrame();

        if (frame < 0) {
            /*
             * Roll back already allocated frames.
             */
            for (
                std::size_t i = 0;
                i < table.size();
                ++i
            ) {
                int allocatedFrame =
                    table.getFrame(i);

                if (allocatedFrame >= 0) {
                    freeFrame(
                        static_cast<std::size_t>(
                            allocatedFrame
                        )
                    );
                }
            }

            return false;
        }

        if (!table.mapPage(
                page,
                static_cast<std::size_t>(frame)
            )) {
            freeFrame(
                static_cast<std::size_t>(frame)
            );

            return false;
        }
    }

    pageTables_.push_back(table);

    return true;
}

bool MemoryManager::freeProcess(
    std::uint32_t pid
) {
    PageTable* table =
        findPageTable(pid);

    if (!table) {
        return false;
    }

    /*
     * Release every physical frame belonging
     * to this process.
     */
    for (
        std::size_t page = 0;
        page < table->size();
        ++page
    ) {
        int frame =
            table->getFrame(page);

        if (frame >= 0) {
            freeFrame(
                static_cast<std::size_t>(
                    frame
                )
            );
        }
    }

    for (
        auto it = pageTables_.begin();
        it != pageTables_.end();
        ++it
    ) {
        if (it->getPid() == pid) {
            pageTables_.erase(it);
            return true;
        }
    }

    return false;
}

std::size_t MemoryManager::translateAddress(
    std::uint32_t pid,
    std::size_t virtualAddress
) const {
    const PageTable* table =
        findPageTable(pid);

    if (!table) {
        return static_cast<std::size_t>(-1);
    }

    std::size_t virtualPage =
        virtualAddress / PAGE_SIZE;

    std::size_t offset =
        virtualAddress % PAGE_SIZE;

    int frame =
        table->getFrame(virtualPage);

    if (frame < 0) {
        return static_cast<std::size_t>(-1);
    }

    return (
        static_cast<std::size_t>(frame)
        * PAGE_SIZE
    ) + offset;
}

bool MemoryManager::write(
    std::uint32_t pid,
    std::size_t virtualAddress,
    std::uint8_t value
) {
    std::size_t physicalAddress =
        translateAddress(
            pid,
            virtualAddress
        );

    if (
        physicalAddress ==
        static_cast<std::size_t>(-1)
    ) {
        return false;
    }

    if (
        physicalAddress >=
        physicalMemory_.size()
    ) {
        return false;
    }

    physicalMemory_[physicalAddress] =
        value;

    return true;
}

bool MemoryManager::read(
    std::uint32_t pid,
    std::size_t virtualAddress,
    std::uint8_t& value
) const {
    std::size_t physicalAddress =
        translateAddress(
            pid,
            virtualAddress
        );

    if (
        physicalAddress ==
        static_cast<std::size_t>(-1)
    ) {
        return false;
    }

    if (
        physicalAddress >=
        physicalMemory_.size()
    ) {
        return false;
    }

    value =
        physicalMemory_[physicalAddress];

    return true;
}

void MemoryManager::printMemoryMap() const {
    std::cout
        << "\n===== Memory Map =====\n";

    for (
        std::size_t i = 0;
        i < frameBitmap_.size();
        ++i
    ) {
        std::cout
            << "Frame "
            << i
            << ": "
            << (
                frameBitmap_[i]
                    ? "USED"
                    : "FREE"
            )
            << '\n';
    }

    std::cout
        << "======================\n";
}

std::size_t
MemoryManager::getPhysicalMemorySize()
const {
    return physicalMemory_.size();
}

std::size_t
MemoryManager::getFreeMemory()
const {
    std::size_t freeFrames = 0;

    for (bool used : frameBitmap_) {
        if (!used) {
            ++freeFrames;
        }
    }

    return freeFrames * PAGE_SIZE;
}
