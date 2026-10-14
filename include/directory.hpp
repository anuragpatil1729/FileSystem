#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

#include <cstdint>
#include <string>
#include <vector>

struct DirectoryEntry {
    std::uint32_t inodeNumber;
    std::string name;
};

class Directory {
public:
    Directory();

    bool addEntry(
        const std::string& name,
        std::uint32_t inodeNumber
    );

    bool removeEntry(
        const std::string& name
    );

    int findEntry(
        const std::string& name
    ) const;

    bool exists(
        const std::string& name
    ) const;

    const std::vector<DirectoryEntry>& getEntries() const;

    std::size_t size() const;

private:
    std::vector<DirectoryEntry> entries_;
};

#endif
