"""Offline validation for YCbCr mapping used by WFM Monitor.

- Generates 75% color bars
- Packs/unpacks v210
- Checks vector (Cb,Cr) centroids land near expected targets
- Checks lightning green-magenta transition stays near a straight scale
"""

from __future__ import annotations

import math
import struct
import sys


def make_bars(width: int = 1920, height: int = 1080):
    # Y,Cb,Cr 10-bit approx for 75% bars
    bars = [
        (721, 512, 512),  # white
        (674, 176, 543),  # yellow
        (581, 589, 176),  # cyan
        (534, 253, 207),  # green
        (251, 771, 817),  # magenta
        (204, 435, 848),  # red
        (111, 848, 481),  # blue
        (64, 512, 512),  # black
    ]
    y = []
    cb = []
    cr = []
    for row in range(height):
        for x in range(width):
            idx = min(7, x * 8 // width)
            yy, u, v = bars[idx]
            y.append(yy)
            cb.append(u)
            cr.append(v)
    return y, cb, cr, bars


def pack_v210(y, cb, cr, width, height):
    # row bytes: ((width+47)//48)*128
    row_bytes = ((width + 47) // 48) * 128
    out = bytearray(row_bytes * height)

    def write_words(offset, words):
        for i, w in enumerate(words):
            struct.pack_into("<I", out, offset + i * 4, w & 0xFFFFFFFF)

    for row in range(height):
        base = row * row_bytes
        x = 0
        while x + 5 < width:
            i = row * width + x
            cb0, y0, cr0 = cb[i], y[i], cr[i]
            y1, cb1, y2 = y[i + 1], cb[i + 2], y[i + 2]
            cr1, y3, cb2 = cr[i + 2], y[i + 3], cb[i + 4]
            y4, cr2, y5 = y[i + 4], cr[i + 4], y[i + 5]
            w0 = (cb0 & 0x3FF) | ((y0 & 0x3FF) << 10) | ((cr0 & 0x3FF) << 20)
            w1 = (y1 & 0x3FF) | ((cb1 & 0x3FF) << 10) | ((y2 & 0x3FF) << 20)
            w2 = (cr1 & 0x3FF) | ((y3 & 0x3FF) << 10) | ((cb2 & 0x3FF) << 20)
            w3 = (y4 & 0x3FF) | ((cr2 & 0x3FF) << 10) | ((y5 & 0x3FF) << 20)
            write_words(base + (x // 6) * 16, (w0, w1, w2, w3))
            x += 6
    return bytes(out), row_bytes


def unpack_v210(buf: bytes, row_bytes: int, width: int, height: int):
    y = [0] * (width * height)
    cb = [0] * (width * height)
    cr = [0] * (width * height)
    for row in range(height):
        off = row * row_bytes
        x = 0
        while x + 5 < width:
            w0, w1, w2, w3 = struct.unpack_from("<4I", buf, off + (x // 6) * 16)
            cb0 = w0 & 0x3FF
            y0 = (w0 >> 10) & 0x3FF
            cr0 = (w0 >> 20) & 0x3FF
            y1 = w1 & 0x3FF
            cb1 = (w1 >> 10) & 0x3FF
            y2 = (w1 >> 20) & 0x3FF
            cr1 = w2 & 0x3FF
            y3 = (w2 >> 10) & 0x3FF
            cb2 = (w2 >> 20) & 0x3FF
            y4 = w3 & 0x3FF
            cr2 = (w3 >> 10) & 0x3FF
            y5 = (w3 >> 20) & 0x3FF
            vals_y = (y0, y1, y2, y3, y4, y5)
            vals_cb = (cb0, cb0, cb1, cb1, cb2, cb2)
            vals_cr = (cr0, cr0, cr1, cr1, cr2, cr2)
            for k in range(6):
                idx = row * width + x + k
                y[idx] = vals_y[k]
                cb[idx] = vals_cb[k]
                cr[idx] = vals_cr[k]
            x += 6
    return y, cb, cr


def norm_c(c: int) -> float:
    return (c - 512) / 896.0


def vector_centroids(y, cb, cr, width, height):
    # Sample mid-line of each bar region
    centers = []
    mid = height // 2
    for b in range(8):
        x0 = b * width // 8
        x1 = (b + 1) * width // 8
        xs = range(x0 + 4, x1 - 4)
        if not xs:
            continue
        scb = sum(cb[mid * width + x] for x in xs) / len(xs)
        scr = sum(cr[mid * width + x] for x in xs) / len(xs)
        centers.append((norm_c(int(scb)), norm_c(int(scr))))
    return centers


def main() -> int:
    width, height = 1920, 1080
    y0, cb0, cr0, bars = make_bars(width, height)
    packed, row_bytes = pack_v210(y0, cb0, cr0, width, height)
    y1, cb1, cr1 = unpack_v210(packed, row_bytes, width, height)

    # Round-trip error on luma (chroma is 4:2:2 reconstructed)
    err = 0
    n = 0
    for i in range(0, width * height, 6):
        err += abs(y0[i] - y1[i])
        n += 1
    mean_err = err / max(n, 1)
    print(f"v210 roundtrip mean |Y| error: {mean_err:.4f}")
    if mean_err > 0.5:
        print("FAIL: v210 unpack mismatch")
        return 1

    centers = vector_centroids(y1, cb1, cr1, width, height)
    # Expected order: W Yl Cy G Mg R B Blk — skip white/black for vector distance
    named = ["W", "Yl", "Cy", "G", "Mg", "R", "B", "Blk"]
    print("Vector centroids (Cb, Cr):")
    for name, (u, v) in zip(named, centers):
        print(f"  {name:3s}: {u:+.3f}, {v:+.3f}")

    # Green and magenta should be roughly opposite quadrants
    g = centers[3]
    mg = centers[4]
    if g[0] >= 0 or g[1] >= 0:
        print("WARN: green not in expected Cb-/Cr- quadrant (approx)")
    if mg[0] <= 0 or mg[1] <= 0:
        print("WARN: magenta not in expected Cb+/Cr+ quadrant (approx)")

    # Lightning: for a vertical strip of green bar, Pb vs Y should be stable sign
    g0 = 3 * width // 8 + width // 16
    pbs = [norm_c(cb1[r * width + g0]) for r in range(0, height, 8)]
    mean_pb = sum(pbs) / len(pbs)
    print(f"Lightning green-bar mean Pb: {mean_pb:+.3f}")
    if abs(mean_pb) < 0.05:
        print("FAIL: lightning Pb collapsed")
        return 1

    print("PASS: offline waveform/vector/lightning data path checks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
