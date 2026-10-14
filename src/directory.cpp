#include "directory.hpp"

Directory::Directory() = default;

bool Directory::addEntry(
    const std::string& name,
    std::uint32_t inodeNumber
) {
    /*
     * Empty filenames are invalid.
     */
    if (name.empty()) {
        return false;
    }

    /*
     * Don't allow duplicate filenames.
     */
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
    for (auto it = entries_.begin();
         it != entries_.end();
         ++it) {

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
            return static_cast<int>(
                entry.inodeNumber
            );
        }
    }

    return -1;
}

bool Directory::exists(
    const std::string& name
) const {
    return findEntry(name) != -1;
}

const std::vector<DirectoryEntry>&
Directory::getEntries() const {
    return entries_;
}

std::size_t Directory::size() const {
    return entries_.size();
}
