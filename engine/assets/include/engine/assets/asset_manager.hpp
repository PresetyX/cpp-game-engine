#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/assets/image.hpp"
#include "engine/assets/mesh.hpp"
#include "engine/assets/result.hpp"
#include "engine/assets/shader_source.hpp"
#include "engine/core/log.hpp"

namespace engine::assets {

// Typed, generational handle. A default handle is invalid. A handle whose asset was released
// (and whose slot may have been reused) is detected by the generation and resolves to nullptr.
template <typename T>
struct Handle {
    std::uint32_t index = 0xFFFFFFFFu;
    std::uint32_t generation = 0;

    bool valid() const { return index != 0xFFFFFFFFu; }
    friend bool operator==(const Handle& a, const Handle& b) {
        return a.index == b.index && a.generation == b.generation;
    }
    friend bool operator!=(const Handle& a, const Handle& b) { return !(a == b); }
};

// Result of a loader: the asset plus every file it was built from (watched for hot reload).
template <typename T>
struct Loaded {
    T value;
    std::vector<std::filesystem::path> files;
};

// Customization point: specialize for a new asset type with
//   static LoadResult<Loaded<T>> load(const std::filesystem::path& root, const std::string& key);
// `key` is already normalized; loaders must not throw for bad input.
template <typename T>
struct AssetLoader;

// Key is a path relative to the manager root, e.g. "textures/brick.png".
template <>
struct AssetLoader<Image> {
    static LoadResult<Loaded<Image>> load(const std::filesystem::path& root,
                                          const std::string& key);
};

// Key is the .obj path.
template <>
struct AssetLoader<Mesh> {
    static LoadResult<Loaded<Mesh>> load(const std::filesystem::path& root,
                                         const std::string& key);
};

// Key is the path WITHOUT extension: "shaders/basic" loads basic.vert and basic.frag.
template <>
struct AssetLoader<ShaderSource> {
    static LoadResult<Loaded<ShaderSource>> load(const std::filesystem::path& root,
                                                 const std::string& key);
};

// Synchronous asset cache with hot reload by file modification time. Loading reads from disk
// and decodes only; sending data to the GPU is the renderer's job, so no window is needed.
//
// - load<T>(key) returns the cached asset if the (normalized) key is already loaded, and counts
//   one reference either way. Each successful load needs one release().
// - get<T>(handle) is nullptr for invalid or stale handles. The pointer stays valid until the
//   asset is reloaded or released; loading other assets does not move it.
// - reload_changed() polls the modification time of every watched file. A changed asset is
//   reloaded in place (same handle, version() incremented). If the reload fails the old data
//   stays and the error is logged; the file is retried only when it changes again.
// Not thread-safe.
class AssetManager {
public:
    explicit AssetManager(std::filesystem::path root) : root_(std::move(root)) {}

    template <typename T>
    LoadResult<Handle<T>> load(const std::string& key) {
        return store<T>().load(root_, key);
    }

    // One reference less; at zero the asset is freed and its handle becomes stale.
    template <typename T>
    void release(Handle<T> handle) {
        store<T>().release(handle);
    }

    template <typename T>
    const T* get(Handle<T> handle) const {
        const Store<T>* s = find_store<T>();
        return s != nullptr ? s->get(handle) : nullptr;
    }

    // 1 after the first load, +1 per successful reload; 0 for invalid or stale handles.
    template <typename T>
    std::uint32_t version(Handle<T> handle) const {
        const Store<T>* s = find_store<T>();
        return s != nullptr ? s->version(handle) : 0;
    }

    template <typename T>
    std::uint32_t ref_count(Handle<T> handle) const {
        const Store<T>* s = find_store<T>();
        return s != nullptr ? s->ref_count(handle) : 0;
    }

    // Number of live assets of type T.
    template <typename T>
    std::size_t size() const {
        const Store<T>* s = find_store<T>();
        return s != nullptr ? s->size() : 0;
    }

    // Returns how many assets were reloaded successfully.
    std::size_t reload_changed() {
        std::size_t n = 0;
        for (auto& entry : stores_) {
            n += entry.second->reload_changed(root_);
        }
        return n;
    }

    const std::filesystem::path& root() const { return root_; }

private:
    using FileTime = std::filesystem::file_time_type;

    struct Watched {
        std::filesystem::path path;
        FileTime time;
    };

    static FileTime time_of(const std::filesystem::path& p) {
        std::error_code ec;
        const FileTime t = std::filesystem::last_write_time(p, ec);
        return ec ? FileTime::min() : t;
    }

    static std::vector<Watched> watch(const std::vector<std::filesystem::path>& files) {
        std::vector<Watched> w;
        w.reserve(files.size());
        for (const auto& f : files) {
            w.push_back({f, time_of(f)});
        }
        return w;
    }

    class StoreBase {
    public:
        virtual ~StoreBase() = default;
        virtual std::size_t reload_changed(const std::filesystem::path& root) = 0;
    };

    template <typename T>
    class Store final : public StoreBase {
    public:
        LoadResult<Handle<T>> load(const std::filesystem::path& root, const std::string& raw_key) {
            const std::string key = std::filesystem::path(raw_key).lexically_normal().generic_string();
            if (key.empty() || key == ".") {
                return LoadResult<Handle<T>>::failure("asset key is empty");
            }
            if (const auto it = by_key_.find(key); it != by_key_.end()) {
                Slot& slot = slots_[it->second];
                ++slot.refs;
                return Handle<T>{it->second, slot.generation};
            }
            auto loaded = AssetLoader<T>::load(root, key);
            if (!loaded) {
                return LoadResult<Handle<T>>::failure(loaded.error());
            }
            std::uint32_t index = 0;
            if (!free_.empty()) {
                index = free_.back();
                free_.pop_back();
            } else {
                index = static_cast<std::uint32_t>(slots_.size());
                slots_.emplace_back();
            }
            Slot& slot = slots_[index];
            slot.value = std::make_unique<T>(std::move(loaded->value));
            slot.files = watch(loaded->files);
            slot.key = key;
            slot.refs = 1;
            slot.version = 1;
            by_key_.emplace(key, index);
            ++live_;
            return Handle<T>{index, slot.generation};
        }

        void release(Handle<T> h) {
            Slot* slot = resolve(h);
            if (slot == nullptr) {
                return;
            }
            if (--slot->refs > 0) {
                return;
            }
            by_key_.erase(slot->key);
            slot->value.reset();
            slot->files.clear();
            slot->key.clear();
            slot->version = 0;
            ++slot->generation;
            free_.push_back(h.index);
            --live_;
        }

        const T* get(Handle<T> h) const {
            const Slot* slot = resolve(h);
            return slot != nullptr ? slot->value.get() : nullptr;
        }
        std::uint32_t version(Handle<T> h) const {
            const Slot* slot = resolve(h);
            return slot != nullptr ? slot->version : 0;
        }
        std::uint32_t ref_count(Handle<T> h) const {
            const Slot* slot = resolve(h);
            return slot != nullptr ? slot->refs : 0;
        }
        std::size_t size() const { return live_; }

        std::size_t reload_changed(const std::filesystem::path& root) override {
            std::size_t reloaded = 0;
            for (Slot& slot : slots_) {
                if (!slot.value) {
                    continue;
                }
                bool changed = false;
                for (const Watched& w : slot.files) {
                    changed = changed || time_of(w.path) != w.time;
                }
                if (!changed) {
                    continue;
                }
                auto loaded = AssetLoader<T>::load(root, slot.key);
                if (!loaded) {
                    // Remember the new times so a broken file is not retried every poll.
                    slot.files = watch(slot.files_paths());
                    log_error("asset reload failed, keeping the old version: ", loaded.error());
                    continue;
                }
                slot.value = std::make_unique<T>(std::move(loaded->value));
                slot.files = watch(loaded->files);
                ++slot.version;
                ++reloaded;
                log_info("asset reloaded: ", slot.key);
            }
            return reloaded;
        }

    private:
        struct Slot {
            std::unique_ptr<T> value;
            std::vector<Watched> files;
            std::string key;
            std::uint32_t generation = 1;
            std::uint32_t refs = 0;
            std::uint32_t version = 0;

            std::vector<std::filesystem::path> files_paths() const {
                std::vector<std::filesystem::path> p;
                p.reserve(files.size());
                for (const Watched& w : files) {
                    p.push_back(w.path);
                }
                return p;
            }
        };

        Slot* resolve(Handle<T> h) {
            return const_cast<Slot*>(std::as_const(*this).resolve(h));
        }
        const Slot* resolve(Handle<T> h) const {
            if (!h.valid() || h.index >= slots_.size()) {
                return nullptr;
            }
            const Slot& slot = slots_[h.index];
            return (slot.generation == h.generation && slot.value) ? &slot : nullptr;
        }

        std::vector<Slot> slots_;
        std::vector<std::uint32_t> free_;
        std::unordered_map<std::string, std::uint32_t> by_key_;
        std::size_t live_ = 0;
    };

    template <typename T>
    Store<T>& store() {
        auto& p = stores_[std::type_index(typeid(T))];
        if (!p) {
            p = std::make_unique<Store<T>>();
        }
        return static_cast<Store<T>&>(*p);
    }

    template <typename T>
    const Store<T>* find_store() const {
        const auto it = stores_.find(std::type_index(typeid(T)));
        return it != stores_.end() ? static_cast<const Store<T>*>(it->second.get()) : nullptr;
    }

    std::filesystem::path root_;
    std::unordered_map<std::type_index, std::unique_ptr<StoreBase>> stores_;
};

}  // namespace engine::assets
