#include "video/V210Unpack.h"

#include <algorithm>
#include <cstring>

namespace V210Unpack {
namespace {

inline uint32_t readLE32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

void resizePlanes(VideoFrame& out, int width, int height)
{
    const size_t n = size_t(width) * size_t(height);
    out.width = width;
    out.height = height;
    out.y.resize(n);
    out.cb.resize(n);
    out.cr.resize(n);
}

} // namespace

bool unpackV210(const uint8_t* src, int rowBytes, int width, int height, VideoFrame& out)
{
    if (!src || width <= 0 || height <= 0 || rowBytes <= 0)
        return false;

    // v210 packs 6 pixels / 16 bytes; row is 128-bit aligned.
    const int minRow = ((width + 47) / 48) * 128;
    if (rowBytes < minRow)
        return false;

    resizePlanes(out, width, height);
    out.pixelFormat = PixelFormat::V210;

    for (int y = 0; y < height; ++y) {
        const uint8_t* row = src + size_t(y) * size_t(rowBytes);
        uint16_t* yDst = out.y.data() + size_t(y) * size_t(width);
        uint16_t* cbDst = out.cb.data() + size_t(y) * size_t(width);
        uint16_t* crDst = out.cr.data() + size_t(y) * size_t(width);

        int x = 0;
        while (x + 5 < width) {
            const uint32_t w0 = readLE32(row + 0);
            const uint32_t w1 = readLE32(row + 4);
            const uint32_t w2 = readLE32(row + 8);
            const uint32_t w3 = readLE32(row + 12);

            const uint16_t cb0 = uint16_t(w0 & 0x3FF);
            const uint16_t y0 = uint16_t((w0 >> 10) & 0x3FF);
            const uint16_t cr0 = uint16_t((w0 >> 20) & 0x3FF);

            const uint16_t y1 = uint16_t(w1 & 0x3FF);
            const uint16_t cb1 = uint16_t((w1 >> 10) & 0x3FF);
            const uint16_t y2 = uint16_t((w1 >> 20) & 0x3FF);

            const uint16_t cr1 = uint16_t(w2 & 0x3FF);
            const uint16_t y3 = uint16_t((w2 >> 10) & 0x3FF);
            const uint16_t cb2 = uint16_t((w2 >> 20) & 0x3FF);

            const uint16_t y4 = uint16_t(w3 & 0x3FF);
            const uint16_t cr2 = uint16_t((w3 >> 10) & 0x3FF);
            const uint16_t y5 = uint16_t((w3 >> 20) & 0x3FF);

            yDst[x + 0] = y0;
            yDst[x + 1] = y1;
            yDst[x + 2] = y2;
            yDst[x + 3] = y3;
            yDst[x + 4] = y4;
            yDst[x + 5] = y5;

            cbDst[x + 0] = cb0;
            cbDst[x + 1] = cb0;
            cbDst[x + 2] = cb1;
            cbDst[x + 3] = cb1;
            cbDst[x + 4] = cb2;
            cbDst[x + 5] = cb2;

            crDst[x + 0] = cr0;
            crDst[x + 1] = cr0;
            crDst[x + 2] = cr1;
            crDst[x + 3] = cr1;
            crDst[x + 4] = cr2;
            crDst[x + 5] = cr2;

            x += 6;
            row += 16;
        }

        // Remainder pixels (rare for broadcast widths which are multiples of 6/2).
        while (x < width) {
            // Pad with last decoded or neutral.
            yDst[x] = (x > 0) ? yDst[x - 1] : 64;
            cbDst[x] = (x > 0) ? cbDst[x - 1] : 512;
            crDst[x] = (x > 0) ? crDst[x - 1] : 512;
            ++x;
        }
    }

    return true;
}

bool unpackUYVY(const uint8_t* src, int rowBytes, int width, int height, VideoFrame& out)
{
    if (!src || width <= 0 || height <= 0 || rowBytes < width * 2)
        return false;

    resizePlanes(out, width, height);
    out.pixelFormat = PixelFormat::UYVY;

    for (int y = 0; y < height; ++y) {
        const uint8_t* row = src + size_t(y) * size_t(rowBytes);
        uint16_t* yDst = out.y.data() + size_t(y) * size_t(width);
        uint16_t* cbDst = out.cb.data() + size_t(y) * size_t(width);
        uint16_t* crDst = out.cr.data() + size_t(y) * size_t(width);

        for (int x = 0; x + 1 < width; x += 2) {
            const uint8_t u = row[0];
            const uint8_t y0 = row[1];
            const uint8_t v = row[2];
            const uint8_t y1 = row[3];
            // Scale 8-bit (16-235/240) into 10-bit-ish by << 2
            yDst[x] = uint16_t(y0) << 2;
            yDst[x + 1] = uint16_t(y1) << 2;
            cbDst[x] = cbDst[x + 1] = uint16_t(u) << 2;
            crDst[x] = crDst[x + 1] = uint16_t(v) << 2;
            row += 4;
        }
        if (width & 1) {
            const int x = width - 1;
            yDst[x] = yDst[x - 1];
            cbDst[x] = cbDst[x - 1];
            crDst[x] = crDst[x - 1];
        }
    }

    return true;
}

} // namespace V210Unpack
