#pragma once

#include "video/VideoFrame.h"

#include <atomic>
#include <mutex>

// Latest-frame queue: capture overwrites; renderer always takes newest.
class FrameQueue {
public:
    void push(VideoFramePtr frame);
    VideoFramePtr takeLatest();
    VideoFramePtr peekLatest() const;

    uint64_t pushed() const { return pushed_.load(std::memory_order_relaxed); }
    uint64_t dropped() const { return dropped_.load(std::memory_order_relaxed); }
    void resetStats();

private:
    mutable std::mutex mutex_;
    VideoFramePtr latest_;
    std::atomic<uint64_t> pushed_{0};
    std::atomic<uint64_t> dropped_{0};
};
