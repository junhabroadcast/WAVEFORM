# Features — WAVEFORM v1.0.0

## Capture

- Blackmagic **DeckLink** SDI input via Desktop Video COM API
- Device enumeration and selection
- Prefer **10-bit YUV (v210)**; fallback to **8-bit UYVY**
- Format detection / reconfigure on input change
- Lock / no-signal status and frame counters
- Latest-frame queue (low latency; drops stale frames)
- **Simulator fallback**: 1080p ≈59.94 75% color bars when DeckLink is unavailable

## Displays

### Waveform
- Voltage-vs-time plot of Y / Cb / Cr
- **Parade** and **Overlay** styles
- **Line** and **Field** sweep
- Component enable/disable (Y, Cb, Cr)
- Graticule with 0 / 50 / 100% style marks

### Vector
- Cb (X) vs Cr (Y) plot
- **75%** / **100%** color-bar target boxes
- Compass / I–Q style axes overlay

### Lightning
- Upper half: **Pb vs Y**
- Lower half: **Pr vs Y**
- Target marks for gain / chroma–luma delay checks (color-bar oriented)

## Rendering

- OpenGL 3.3 core profile
- Per-sample GPU point mapping from planar YCbCr textures
- Additive accumulation into **R32F** buffer
- Persistence (decay) control
- Intensity / CRT green tonemap

## UI / controls

- 1 / 2 / 4 tile layouts
- Per-tile mode assignment (Waveform / Vector / Lightning)
- Gain: 1x / 2x / 5x / variable
- Horizontal Mag: 1x / 5x / 10x / 20x
- Line Select
- Freeze
- Persistence and intensity sliders
- Status bar: lock state, mode name, push/drop/capture counters

## Tooling

- CMake + Ninja + MSVC build
- `tools/build.ps1` helper
- `tools/validate_offline.py` for v210 round-trip and vector/lightning sanity checks
