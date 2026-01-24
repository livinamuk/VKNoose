#pragma once
#include <atomic>

struct LoadingState {
    enum struct Value : uint8_t {
        AWAITING_LOADING_FROM_DISK,
        LOADING_FROM_DISK,
        LOADING_COMPLETE
    };

    LoadingState() noexcept = default;

    constexpr LoadingState(Value init) noexcept : m_value(init) {}

    LoadingState(const LoadingState& rhs) noexcept
        : m_value(rhs.m_value.load(std::memory_order_relaxed)) {
    }

    LoadingState& operator=(const LoadingState& rhs) noexcept {
        m_value.store(rhs.m_value.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    LoadingState(LoadingState&& rhs) noexcept
        : m_value(rhs.m_value.load(std::memory_order_relaxed)) {
    }

    LoadingState& operator=(LoadingState&& rhs) noexcept {
        m_value.store(rhs.m_value.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    void SetLoadingState(LoadingState::Value state) {
        m_value.store(state, std::memory_order_release);
    }

    Value GetLoadingState() const {
        return m_value.load(std::memory_order_acquire);
    }

    LoadingState& operator=(Value state) noexcept {
        SetLoadingState(state); return *this;
    }

    friend bool operator==(const LoadingState& a, Value b) noexcept {
        return a.m_value.load(std::memory_order_acquire) == b;
    }

    friend bool operator==(Value a, const LoadingState& b) noexcept {
        return b == a;
    }

    friend bool operator!=(const LoadingState& a, Value b) noexcept {
        return !(a == b);
    }

    friend bool operator!=(Value a, const LoadingState& b) noexcept {
        return !(b == a);
    }

private:
    std::atomic<LoadingState::Value> m_value{ LoadingState::Value::AWAITING_LOADING_FROM_DISK };
};