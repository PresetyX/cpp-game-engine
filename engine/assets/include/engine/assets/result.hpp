#pragma once

#include <optional>
#include <string>
#include <utility>

namespace engine::assets {

// Value or error message. Loaders never throw for bad input or missing files.
template <typename T>
class LoadResult {
public:
    LoadResult(T value) : value_(std::move(value)) {}  // NOLINT: implicit on purpose

    static LoadResult failure(std::string message) {
        LoadResult r;
        r.error_ = std::move(message);
        return r;
    }

    explicit operator bool() const { return value_.has_value(); }
    bool ok() const { return value_.has_value(); }

    // Precondition: ok().
    T& value() { return *value_; }
    const T& value() const { return *value_; }
    T& operator*() { return *value_; }
    const T& operator*() const { return *value_; }
    T* operator->() { return &*value_; }
    const T* operator->() const { return &*value_; }

    // Empty when ok().
    const std::string& error() const { return error_; }

private:
    LoadResult() = default;

    std::optional<T> value_;
    std::string error_;
};

}  // namespace engine::assets
