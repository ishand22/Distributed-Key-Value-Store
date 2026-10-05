#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace dkv {

// Thread-safe, WAL-backed in-memory LSM-style engine. The compacted sorted
// snapshot is the immutable disk level; the WAL restores newer mutations.
class Engine {
public:
    explicit Engine(std::filesystem::path directory);
    void put(const std::string& key, const std::string& value);
    bool erase(const std::string& key);
    [[nodiscard]] std::optional<std::string> get(const std::string& key) const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> scan() const;
    void compact();
    [[nodiscard]] std::size_t size() const;

private:
    struct Entry { std::string value; bool deleted = false; };
    void recover();
    void append(char operation, const std::string& key, const std::string& value);
    static std::string encode(const std::string& value);
    static std::optional<std::string> decode(const std::string& value);
    std::filesystem::path directory_, wal_, snapshot_;
    mutable std::mutex mutex_;
    std::map<std::string, Entry> data_;
};
}
