#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct DirectoryEntry {
    std::uint32_t inodeNumber;
    std::string name;
};

class Directory {
public:
    static constexpr std::size_t ENTRY_SIZE = 128;
    static constexpr std::size_t MAX_NAME_LENGTH = 120;

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
    bool empty() const;
    void clear();

    std::vector<std::uint8_t> serialize() const;
    bool deserialize(const std::uint8_t* data, std::size_t byteCount);

private:
    std::vector<DirectoryEntry> entries_;
};

#endif
