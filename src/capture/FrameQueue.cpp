#include "capture/FrameQueue.h"

void FrameQueue::push(VideoFramePtr frame)
{
    if (!frame)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (latest_)
        dropped_.fetch_add(1, std::memory_order_relaxed);
    latest_ = std::move(frame);
    pushed_.fetch_add(1, std::memory_order_relaxed);
}

VideoFramePtr FrameQueue::takeLatest()
{
    std::lock_guard<std::mutex> lock(mutex_);
    VideoFramePtr out = latest_;
    latest_.reset();
    return out;
}

VideoFramePtr FrameQueue::peekLatest() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_;
}

void FrameQueue::resetStats()
{
    pushed_.store(0, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
}
