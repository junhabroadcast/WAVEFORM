# WAVEFORM

**버전:** v2.0.0

Windows용 방송급 SDI 웨이브폼 모니터입니다. Blackmagic DeckLink 카드로 입력을 받아 **Waveform**, **Vector**, **Lightning**, **Video(화면)** 를 실시간으로 표시합니다.

## 주요 기능

- DeckLink SDI 입력 (10-bit v210 우선, 8-bit UYVY 폴백)
- **Color Bars** 토글 — 캡처카드 없이 내장 75% 컬러바로 QC 가능
- OpenGL 스코프 (CRT형 persistence / 녹색 톤맵)
- **Video 타일** — 스코프와 같은 프레임의 실시간 화면
- **Vector / Lightning 연속 트레이스** — 샘플을 선분으로 연결해 컬러바도 또렷하게 표시
- **정확한 타깃 박스** — BT.601 / BT.709 색 행렬로 계산, 트레이스와 계수선 좌표 일치
- **I/Q 축** — SD(BT.601)에서만 표시 (HD 컴포넌트에서는 숨김)
- 1 / 2 / 4 타일 레이아웃 (기본: Waveform / Vector / Lightning / Video)
- Gain, Mag, Line Select, Freeze, Persistence, Intensity

## 요구 사항

- Windows 10/11 x64
- Blackmagic Desktop Video 드라이버 + DeckLink 카드
- Visual Studio 2022 Build Tools (MSVC)
- CMake ≥ 3.21
- Qt 6.7+ (Widgets, OpenGL, OpenGLWidgets)
- OpenGL 3.3 이상 GPU

## 빠른 실행

```powershell
.\tools\build.ps1
.\build\WfmMonitor.exe
```

수동 빌드:

```powershell
$QtRoot = "C:\Qt\6.7.3\msvc2019_64"
# x64 Native Tools / vcvars64.bat 실행 후
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QtRoot"
cmake --build build --config Release
& "$QtRoot\bin\windeployqt.exe" --release --no-translations build\WfmMonitor.exe
.\build\WfmMonitor.exe
```

**Start**를 누르면 동작합니다. SDI가 없거나 **Color Bars**를 켜면 내장 컬러바를 사용합니다.

## 문서

| 파일 | 설명 |
|------|------|
| [features.md](features.md) | v2.0.0 기능 목록 |
| [notes.md](notes.md) | 구조, 제한, 사용 팁 |
| [changelog.md](changelog.md) | 변경 이력 |

## 오프라인 검증

```powershell
python tools\validate_offline.py
```

## 라이선스

저장소 소유자가 별도로 명시하지 않는 한 저작권은 소유자에게 있습니다.
