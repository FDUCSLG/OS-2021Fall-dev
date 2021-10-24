#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

template <typename Key, typename Value>
class Map {
public:
    template <typename... Args>
    void add(const Key &key, Args &&...args) {
        std::unique_lock lock(mutex);
        if (!map.try_emplace(key, std::forward<Args>(args)...).second)
            throw std::runtime_error("key already exists");
    }

    auto operator[](const Key &key) -> Value & {
        std::shared_lock lock(mutex);
        auto it = map.find(key);
        if (it == map.end())
            throw std::runtime_error("key not found");
        return it->second;
    }

private:
    std::shared_mutex mutex;
    std::unordered_map<Key, Value> map;
};