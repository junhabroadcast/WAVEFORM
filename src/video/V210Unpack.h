#pragma once

#include "video/VideoFrame.h"

#include <cstdint>

namespace V210Unpack {

// Unpack Blackmagic bmdFormat10BitYUV (v210) into planar uint16 Y/Cb/Cr.
// width/height are active picture dimensions; rowBytes is the DeckLink stride.
bool unpackV210(const uint8_t* src, int rowBytes, int width, int height, VideoFrame& out);

// Unpack bmdFormat8BitYUV (UYVY) into planar uint16 (values scaled to 10-bit range * 4).
bool unpackUYVY(const uint8_t* src, int rowBytes, int width, int height, VideoFrame& out);

} // namespace V210Unpack
