# Notes — WAVEFORM v1.0.0

## Scope

v1.0.0 focuses on **picture-layer** analysis only:

- Waveform, Vector, Lightning

Not included (by design for this release):

- Eye pattern / jitter (needs specialized PHY hardware; not available from deserialized SDI alone)
- Diamond / Arrowhead / Bowtie
- Audio meters / surround
- ANC / closed caption deep decode
- Composite simulation mode

## Architecture (short)

```
SDI → DeckLink callback → FrameQueue → v210/UYVY unpack → Y/Cb/Cr planes
  → OpenGL textures → vertex-ID point shader (mode mapping)
  → additive R32F accum (+ decay) → CRT tonemap → Qt graticule overlay
```

Key modules:

| Path | Role |
|------|------|
| `src/capture/` | DeckLink COM capture + queue |
| `src/video/` | v210 / UYVY unpack |
| `src/render/` | OpenGL accumulate / tonemap widget |
| `src/display/` | Modes + graticule drawing |
| `src/ui/` | Main window + tile controls |
| `third_party/decklink/` | `#import` of installed `DeckLinkAPI64.dll` |

## Build / runtime tips

1. Install **Desktop Video** so `C:\Program Files\Blackmagic Design\Desktop Video\DeckLinkAPI64.dll` exists. The Windows build imports that typelib; a separate SDK zip is not required for v1.0.0.
2. Qt path in `tools/build.ps1` defaults to `C:\Qt\6.7.3\msvc2019_64`. Edit if your install differs.
3. Run `windeployqt` (done by the build script) before moving the exe to another machine.
4. First launch: select device → **Start**. Status should show `LOCKED …` with live SDI, or `SIMULATOR …` offline.

## Performance expectations

- Target: 1080i/p class with full-sample accumulation (no intentional downsampling).
- Heavy multi-tile + high persistence can increase GPU load; reduce tile count if needed.
- Queue keeps only the newest frame to bound latency.

## Colorimetry

- Auto chooses BT.601 vs BT.709 from active height (≥720 → BT.709).
- Vector / Lightning target positions are approximate studio-bar locations for visual QC, not a certified legalizer.

## Manuals in the workspace

Tektronix PDF manuals used for design reference may live next to the project locally. They are **not** part of the GitHub release bundle (large binaries / third-party copyright).

## Known limitations

- `#import` / MSVC is required on Windows for DeckLink COM.
- Simulator bars are for UI/path validation, not for calibrating a production chain.
- RGB parade / composite gamut views are deferred to a later version.
