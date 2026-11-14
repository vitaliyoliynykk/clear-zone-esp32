#pragma once
#include <algorithm>
#include <cstdint>

class RetryBackoff
{
public:
    constexpr RetryBackoff(uint32_t initialMs, uint32_t maxMs)
        : retryAt_(0), backoffMs_(initialMs), initialMs_(initialMs), maxMs_(maxMs) {}

    bool isDue(uint32_t now) const
    {
        return static_cast<int32_t>(now - retryAt_) >= 0;
    }

    void reset()
    {
        retryAt_ = 0;
        backoffMs_ = initialMs_;
    }

    void schedule(uint32_t now)
    {
        retryAt_ = now + backoffMs_;
        backoffMs_ = std::min(backoffMs_ * 2U, maxMs_);
    }

    void setDeadline(uint32_t retryAt)
    {
        retryAt_ = retryAt;
    }

private:
    uint32_t retryAt_;
    uint32_t backoffMs_;
    const uint32_t initialMs_;
    const uint32_t maxMs_;
};
