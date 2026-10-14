#include "directory.hpp"
#include "byte_io.hpp"

#include <cstring>
#include <unordered_set>

Directory::Directory() = default;

bool Directory::addEntry(
    const std::string& name,
    std::uint32_t inodeNumber
) {
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        return false;
    }

    // Prohibit '/' or null in entry names
    if (name.find('/') != std::string::npos || name.find('\0') != std::string::npos) {
        return false;
    }

    if (exists(name)) {
        return false;
    }

    DirectoryEntry entry;
    entry.inodeNumber = inodeNumber;
    entry.name = name;

    entries_.push_back(entry);
    return true;
}

bool Directory::removeEntry(
    const std::string& name
) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->name == name) {
            entries_.erase(it);
            return true;
        }
    }
    return false;
}

int Directory::findEntry(
    const std::string& name
) const {
    for (const auto& entry : entries_) {
        if (entry.name == name) {
            return static_cast<int>(entry.inodeNumber);
        }
    }
    return -1;
}

bool Directory::exists(
    const std::string& name
) const {
    return findEntry(name) != -1;
}

const std::vector<DirectoryEntry>& Directory::getEntries() const {
    return entries_;
}

std::size_t Directory::size() const {
    return entries_.size();
}

bool Directory::empty() const {
    return entries_.empty();
}

void Directory::clear() {
    entries_.clear();
}

std::vector<std::uint8_t> Directory::serialize() const {
    std::vector<std::uint8_t> buffer(entries_.size() * ENTRY_SIZE, 0);
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        std::uint8_t* slot = buffer.data() + i * ENTRY_SIZE;
        byteio::putU32(slot, entries_[i].inodeNumber);
        const auto len = static_cast<std::uint32_t>(entries_[i].name.size());
        byteio::putU32(slot + 4, len);
        std::memcpy(slot + 8, entries_[i].name.data(), len);
    }
    return buffer;
}

bool Directory::deserialize(const std::uint8_t* data, std::size_t byteCount) {
    clear();
    if (byteCount == 0) {
        return true;
    }
    if (byteCount % ENTRY_SIZE != 0) {
        return false;
    }

    const std::size_t count = byteCount / ENTRY_SIZE;
    std::unordered_set<std::string> seenNames;

    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t* slot = data + i * ENTRY_SIZE;
        const std::uint32_t inodeNum = byteio::getU32(slot);
        const std::uint32_t nameLen = byteio::getU32(slot + 4);

        if (nameLen == 0 || nameLen > MAX_NAME_LENGTH) {
            clear();
            return false;
        }

        std::string name(reinterpret_cast<const char*>(slot + 8), nameLen);
        if (name.find('/') != std::string::npos || name.find('\0') != std::string::npos) {
            clear();
            return false;
        }

        if (seenNames.count(name) > 0) {
            clear();
            return false; // Duplicate name in directory
        }
        seenNames.insert(name);

        DirectoryEntry entry;
        entry.inodeNumber = inodeNum;
        entry.name = std::move(name);
        entries_.push_back(std::move(entry));
    }

    return true;
}
