# WAVEFORM

**Version:** v1.0.0

Broadcast-oriented SDI waveform monitor for Windows. Captures video from a Blackmagic DeckLink card and renders **Waveform**, **Vector**, and **Lightning** displays with CRT-style persistence.

## Highlights

- DeckLink SDI input with auto format detection (10-bit v210 preferred)
- Offline **75% color bar simulator** when no device/signal is present
- GPU point accumulation (OpenGL 3.3) with additive blend + green phosphor tonemap
- Multi-tile UI (1 / 2 / 4) with Gain, Mag, Line Select, Freeze, Persistence

## Requirements

- Windows 10/11 x64
- Blackmagic Desktop Video drivers + DeckLink card
- Visual Studio 2022 Build Tools (MSVC)
- CMake ≥ 3.21
- Qt 6.7+ (Widgets, OpenGL, OpenGLWidgets)
- OpenGL 3.3+ capable GPU

## Quick start

```powershell
.\tools\build.ps1
.\build\WfmMonitor.exe
```

Or manually:

```powershell
$QtRoot = "C:\Qt\6.7.3\msvc2019_64"
# Open "x64 Native Tools" / run vcvars64.bat first
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QtRoot"
cmake --build build --config Release
& "$QtRoot\bin\windeployqt.exe" --release --no-translations build\WfmMonitor.exe
.\build\WfmMonitor.exe
```

Press **Start**. If no SDI lock is available, the app falls back to the simulator.

## Documentation

| File | Description |
|------|-------------|
| [features.md](features.md) | Feature list for v1.0.0 |
| [notes.md](notes.md) | Architecture notes, limits, setup tips |
| [changelog.md](changelog.md) | Release history |

## Offline validation

```powershell
python tools\validate_offline.py
```

## License

Proprietary / all rights reserved unless otherwise stated by the repository owner.
