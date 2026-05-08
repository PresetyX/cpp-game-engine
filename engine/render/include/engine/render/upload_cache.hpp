#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::render {

// Remembers which version of an asset is on the GPU. Generic over the handle type (anything with
// `index` and `generation`, like assets::Handle<T>) and the GPU object type, so the logic is
// testable with plain ints and no OpenGL.
//
// Typical frame:
//   if (cache.needs_upload(h, assets.version(h))) { cache.store(h, version, GpuMesh(*cpu)); }
//   ... draw with *cache.find(h) ...
//   cache.collect_stale([&](Handle h) { return assets.version(h); });   // once per frame
//
// Storing over an existing entry destroys the old GPU object (hot reload). Not thread-safe.
template <typename Handle, typename Gpu>
class UploadCache {
public:
    // True when nothing is stored for the handle or the stored version differs. Version 0 (a
    // released/stale handle) never needs an upload.
    bool needs_upload(Handle handle, std::uint32_t version) const {
        if (version == 0) {
            return false;
        }
        const auto it = entries_.find(key_of(handle));
        return it == entries_.end() || it->second.version != version;
    }

    // Replaces the entry; the returned reference stays valid until the entry is replaced or erased.
    Gpu& store(Handle handle, std::uint32_t version, Gpu gpu) {
        auto it = entries_.find(key_of(handle));
        if (it == entries_.end()) {
            it = entries_.emplace(key_of(handle), Entry{handle, version, std::move(gpu)}).first;
        } else {
            it->second.version = version;
            it->second.gpu = std::move(gpu);
        }
        return it->second.gpu;
    }

    Gpu* find(Handle handle) {
        const auto it = entries_.find(key_of(handle));
        return it != entries_.end() ? &it->second.gpu : nullptr;
    }
    const Gpu* find(Handle handle) const {
        const auto it = entries_.find(key_of(handle));
        return it != entries_.end() ? &it->second.gpu : nullptr;
    }

    std::uint32_t stored_version(Handle handle) const {
        const auto it = entries_.find(key_of(handle));
        return it != entries_.end() ? it->second.version : 0;
    }

    // version_of(handle) must return the current asset version, 0 if released. Entries whose
    // asset is gone are erased (their GPU objects are destroyed). Returns how many were erased.
    template <typename VersionOf>
    std::size_t collect_stale(VersionOf&& version_of) {
        std::size_t erased = 0;
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (version_of(it->second.handle) == 0) {
                it = entries_.erase(it);
                ++erased;
            } else {
                ++it;
            }
        }
        return erased;
    }

    std::size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }

private:
    struct Entry {
        Handle handle;
        std::uint32_t version;
        Gpu gpu;
    };

    static std::uint64_t key_of(Handle h) {
        return (static_cast<std::uint64_t>(h.generation) << 32) | static_cast<std::uint64_t>(h.index);
    }

    std::unordered_map<std::uint64_t, Entry> entries_;
};

}  // namespace engine::render
