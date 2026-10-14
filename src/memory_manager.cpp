#include "memory_manager.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
constexpr std::size_t INVALID_ADDR = std::numeric_limits<std::size_t>::max();
}

MemoryManager::MemoryManager(std::size_t physicalMemorySize)
    : physicalMemory_(physicalMemorySize, 0),
      frameBitmap_(physicalMemorySize / PAGE_SIZE, false),
      frameMetadata_(physicalMemorySize / PAGE_SIZE, FrameMetadata{0, 0, 0, 0}),
      allocationPolicy_(AllocationPolicy::EAGER),
      replacementPolicy_(ReplacementPolicy::FIFO),
      timerCounter_(0),
      pageFaultCount_(0),
      pageReplacementCount_(0) {
}

int MemoryManager::allocateFrame() {
    for (std::size_t i = 0; i < frameBitmap_.size(); ++i) {
        if (!frameBitmap_[i]) {
            frameBitmap_[i] = true;
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MemoryManager::freeFrame(std::size_t frameNumber) {
    if (frameNumber >= frameBitmap_.size()) {
        return;
    }
    frameBitmap_[frameNumber] = false;
    frameMetadata_[frameNumber] = FrameMetadata{0, 0, 0, 0};
    // Zero out physical memory of the released frame
    std::memset(physicalMemory_.data() + frameNumber * PAGE_SIZE, 0, PAGE_SIZE);
}

int MemoryManager::selectVictimFrame() const {
    int victim = -1;
    std::size_t bestMetric = std::numeric_limits<std::size_t>::max();

    for (std::size_t i = 0; i < frameBitmap_.size(); ++i) {
        if (!frameBitmap_[i]) continue;

        const std::size_t metric = (replacementPolicy_ == ReplacementPolicy::FIFO)
                                       ? frameMetadata_[i].allocationTime
                                       : frameMetadata_[i].lastAccessTime;
        if (metric < bestMetric) {
            bestMetric = metric;
            victim = static_cast<int>(i);
        }
    }

    return victim;
}

PageTable* MemoryManager::findPageTable(std::uint32_t pid) {
    for (auto& table : pageTables_) {
        if (table.getPid() == pid) {
            return &table;
        }
    }
    return nullptr;
}

const PageTable* MemoryManager::findPageTable(std::uint32_t pid) const {
    for (const auto& table : pageTables_) {
        if (table.getPid() == pid) {
            return &table;
        }
    }
    return nullptr;
}

bool MemoryManager::allocateProcess(std::uint32_t pid, std::size_t virtualMemorySize) {
    if (virtualMemorySize == 0) {
        return false;
    }

    // Check integer overflow on virtual page calculation
    if (virtualMemorySize > std::numeric_limits<std::size_t>::max() - PAGE_SIZE) {
        return false;
    }

    // Duplicate process allocation protection
    if (findPageTable(pid) != nullptr) {
        return false;
    }

    const std::size_t pageCount = (virtualMemorySize + PAGE_SIZE - 1) / PAGE_SIZE;
    PageTable table(pid, virtualMemorySize);

    if (allocationPolicy_ == AllocationPolicy::DEMAND_PAGING) {
        // Under demand paging, initialize unmapped page entries without consuming frames up-front
        for (std::size_t page = 0; page < pageCount; ++page) {
            table.mapPage(page, 0, true);
            table.unmapPage(page); // Mark not present
        }
        pageTables_.push_back(std::move(table));
        return true;
    }

    // EAGER allocation
    for (std::size_t page = 0; page < pageCount; ++page) {
        const int frame = allocateFrame();
        if (frame < 0) {
            // Roll back already allocated frames
            for (std::size_t i = 0; i < table.size(); ++i) {
                const int allocated = table.getFrame(i);
                if (allocated >= 0) {
                    freeFrame(static_cast<std::size_t>(allocated));
                }
            }
            return false;
        }

        if (!table.mapPage(page, static_cast<std::size_t>(frame), true)) {
            freeFrame(static_cast<std::size_t>(frame));
            for (std::size_t i = 0; i < table.size(); ++i) {
                const int allocated = table.getFrame(i);
                if (allocated >= 0) {
                    freeFrame(static_cast<std::size_t>(allocated));
                }
            }
            return false;
        }

        const auto now = ++timerCounter_;
        frameMetadata_[frame] = FrameMetadata{pid, page, now, now};
    }

    pageTables_.push_back(std::move(table));
    return true;
}

bool MemoryManager::freeProcess(std::uint32_t pid) {
    PageTable* table = findPageTable(pid);
    if (!table) {
        return false;
    }

    for (std::size_t page = 0; page < table->size(); ++page) {
        const int frame = table->getFrame(page);
        if (frame >= 0) {
            freeFrame(static_cast<std::size_t>(frame));
        }
    }

    for (auto it = pageTables_.begin(); it != pageTables_.end(); ++it) {
        if (it->getPid() == pid) {
            pageTables_.erase(it);
            return true;
        }
    }

    return false;
}

bool MemoryManager::handlePageFault(std::uint32_t pid, std::size_t virtualPage) {
    PageTable* table = findPageTable(pid);
    if (!table) {
        return false;
    }

    if (virtualPage >= (table->getVirtualMemorySize() + PAGE_SIZE - 1) / PAGE_SIZE) {
        return false; // Out of bounds
    }

    ++pageFaultCount_;

    int frame = allocateFrame();
    if (frame < 0) {
        // No free frames: trigger page replacement
        const int victim = selectVictimFrame();
        if (victim < 0) {
            return false;
        }

        const auto& meta = frameMetadata_[victim];
        PageTable* victimTable = findPageTable(meta.pid);
        if (victimTable) {
            victimTable->unmapPage(meta.virtualPage);
        }

        ++pageReplacementCount_;
        frame = victim;
        std::memset(physicalMemory_.data() + frame * PAGE_SIZE, 0, PAGE_SIZE);
    }

    const auto now = ++timerCounter_;
    frameMetadata_[frame] = FrameMetadata{pid, virtualPage, now, now};
    table->unmapPage(virtualPage); // Ensure unmapped before mapping
    return table->mapPage(virtualPage, static_cast<std::size_t>(frame), true);
}

std::size_t MemoryManager::translateAddress(std::uint32_t pid,
                                           std::size_t virtualAddress) const {
    const PageTable* table = findPageTable(pid);
    if (!table) {
        return INVALID_ADDR;
    }

    if (virtualAddress >= table->getVirtualMemorySize()) {
        return INVALID_ADDR; // Virtual address bounds validation
    }

    const std::size_t virtualPage = virtualAddress / PAGE_SIZE;
    const std::size_t offset = virtualAddress % PAGE_SIZE;

    const int frame = table->getFrame(virtualPage);
    if (frame < 0) {
        return INVALID_ADDR;
    }

    const std::size_t phys = static_cast<std::size_t>(frame) * PAGE_SIZE + offset;
    if (phys >= physicalMemory_.size()) {
        return INVALID_ADDR;
    }

    return phys;
}

std::size_t MemoryManager::translateOrFault(std::uint32_t pid,
                                            std::size_t virtualAddress,
                                            bool isWrite) {
    PageTable* table = findPageTable(pid);
    if (!table) {
        return INVALID_ADDR;
    }

    if (virtualAddress >= table->getVirtualMemorySize()) {
        return INVALID_ADDR;
    }

    const std::size_t virtualPage = virtualAddress / PAGE_SIZE;
    const std::size_t offset = virtualAddress % PAGE_SIZE;

    int frame = table->getFrame(virtualPage);
    if (frame < 0) {
        // Page fault
        if (!handlePageFault(pid, virtualPage)) {
            return INVALID_ADDR;
        }
        frame = table->getFrame(virtualPage);
        if (frame < 0) {
            return INVALID_ADDR;
        }
    }

    PageTableEntry* entry = table->getEntry(virtualPage);
    if (isWrite && entry && !entry->writable) {
        return INVALID_ADDR; // Permission violation
    }

    const auto now = ++timerCounter_;
    if (entry) {
        entry->lastAccessTime = now;
        if (isWrite) entry->dirty = true;
    }
    frameMetadata_[frame].lastAccessTime = now;

    return static_cast<std::size_t>(frame) * PAGE_SIZE + offset;
}

bool MemoryManager::write(std::uint32_t pid,
                          std::size_t virtualAddress,
                          std::uint8_t value) {
    std::size_t phys = translateAddress(pid, virtualAddress);
    if (phys == INVALID_ADDR) {
        // Try demand paging fault handling if present
        phys = translateOrFault(pid, virtualAddress, true);
        if (phys == INVALID_ADDR) {
            return false;
        }
    } else {
        // Record write access
        const PageTable* table = findPageTable(pid);
        if (table) {
            const std::size_t page = virtualAddress / PAGE_SIZE;
            const int frame = table->getFrame(page);
            if (frame >= 0) {
                const auto now = ++timerCounter_;
                frameMetadata_[frame].lastAccessTime = now;
            }
        }
    }

    if (phys >= physicalMemory_.size()) {
        return false;
    }

    physicalMemory_[phys] = value;
    return true;
}

bool MemoryManager::read(std::uint32_t pid,
                         std::size_t virtualAddress,
                         std::uint8_t& value) {
    std::size_t phys = translateAddress(pid, virtualAddress);
    if (phys == INVALID_ADDR) {
        // Try demand paging fault handling
        phys = translateOrFault(pid, virtualAddress, false);
        if (phys == INVALID_ADDR) {
            return false;
        }
    } else {
        const PageTable* table = findPageTable(pid);
        if (table) {
            const std::size_t page = virtualAddress / PAGE_SIZE;
            const int frame = table->getFrame(page);
            if (frame >= 0) {
                const auto now = ++timerCounter_;
                frameMetadata_[frame].lastAccessTime = now;
            }
        }
    }

    if (phys >= physicalMemory_.size()) {
        return false;
    }

    value = physicalMemory_[phys];
    return true;
}

void MemoryManager::printMemoryMap() const {
    std::cout << "\n===== Memory Map =====\n";
    for (std::size_t i = 0; i < frameBitmap_.size(); ++i) {
        std::cout << "Frame " << i << ": ";
        if (frameBitmap_[i]) {
            std::cout << "USED (PID " << frameMetadata_[i].pid
                      << ", VPage " << frameMetadata_[i].virtualPage << ")\n";
        } else {
            std::cout << "FREE\n";
        }
    }
    std::cout << "======================\n";
}

std::size_t MemoryManager::getPhysicalMemorySize() const {
    return physicalMemory_.size();
}

std::size_t MemoryManager::getFreeMemory() const {
    std::size_t freeFrames = 0;
    for (bool used : frameBitmap_) {
        if (!used) ++freeFrames;
    }
    return freeFrames * PAGE_SIZE;
}

void MemoryManager::setAllocationPolicy(AllocationPolicy policy) {
    allocationPolicy_ = policy;
}

AllocationPolicy MemoryManager::getAllocationPolicy() const {
    return allocationPolicy_;
}

void MemoryManager::setReplacementPolicy(ReplacementPolicy policy) {
    replacementPolicy_ = policy;
}

ReplacementPolicy MemoryManager::getReplacementPolicy() const {
    return replacementPolicy_;
}

std::size_t MemoryManager::getPageFaultCount() const {
    return pageFaultCount_;
}

std::size_t MemoryManager::getPageReplacementCount() const {
    return pageReplacementCount_;
}

void MemoryManager::resetStatistics() {
    pageFaultCount_ = 0;
    pageReplacementCount_ = 0;
}
