#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class PixelFormat {
    Unknown,
    V210,   // 10-bit YUV 4:2:2
    UYVY,   // 8-bit YUV 4:2:2 (bmdFormat8BitYUV)
};

enum class Colorimetry {
    Auto,
    BT601,
    BT709,
};

struct VideoFrame {
    int width = 0;
    int height = 0;
    int64_t frameNumber = 0;
    PixelFormat pixelFormat = PixelFormat::Unknown;
    std::string modeName;
    double frameRate = 0.0;
    bool interlaced = false;
    bool hasSignal = false;

    // Planar 10-bit values stored as uint16 (valid range typically 64-940/960)
    std::vector<uint16_t> y;
    std::vector<uint16_t> cb;
    std::vector<uint16_t> cr;

    bool empty() const { return width <= 0 || height <= 0 || y.empty(); }
};

using VideoFramePtr = std::shared_ptr<VideoFrame>;
