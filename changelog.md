# Changelog

All notable changes to **WAVEFORM** are documented here.

## [1.0.0] — 2026-08-12

### Added

- Initial public release **v1.0.0**
- Blackmagic DeckLink SDI capture (v210 / UYVY) with format detection
- Simulator fallback (75% color bars) when no DeckLink signal is available
- Waveform display: Parade / Overlay, Line / Field, component toggles, graticule
- Vector display: Cb/Cr plot with 75% / 100% targets
- Lightning display: Pb–Y / Pr–Y with target marks
- OpenGL CRT-style persistence (additive accumulation + decay + tonemap)
- Multi-tile UI (1 / 2 / 4) with Gain, Mag, Line Select, Freeze, Persistence, Intensity
- Build helpers: CMake project, `tools/build.ps1`, `tools/validate_offline.py`
- Project docs: `README.md`, `features.md`, `notes.md`, `changelog.md`
