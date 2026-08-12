# Changelog

**WAVEFORM**의 주요 변경 사항을 기록합니다.

## [1.1.0] — 2026-08-12

### 추가

- **Video (Picture)** 디스플레이 — 현재 SDI/시뮬레이터 프레임을 YCbCr→RGB로 표시
- 화면비 유지 레터박스, BT.601/BT.709 Auto, Line Select 하이라이트
- **Color Bars** 토글 — DeckLink 대신 내장 75% 컬러바 강제
- 기본 4타일 레이아웃: Waveform / Vector / Lightning / Video

### 수정

- 타일 하단 상태 문구 잔상(글자 겹침) 수정 — OpenGL 위 QPainter 대신 QLabel 사용
- 상태바 `75%%` 표기 → `75%`

### 문서

- README / features / notes / changelog 한글화

## [1.0.0] — 2026-08-12

### 추가

- 최초 공개 릴리스 **v1.0.0**
- Blackmagic DeckLink SDI 캡처 (v210 / UYVY), 포맷 감지
- 장치/신호 없을 때 75% 컬러바 시뮬레이터 폴백
- Waveform: Parade / Overlay, Line / Field, 성분 토글, 계수선
- Vector: Cb/Cr + 75%/100% 타깃
- Lightning: Pb–Y / Pr–Y + 타깃 마크
- OpenGL CRT형 persistence (가산 누적 + decay + 톤맵)
- 멀티타일 UI (1/2/4), Gain / Mag / Line Select / Freeze / Persistence / Intensity
- 빌드 도구: CMake, `tools/build.ps1`, `tools/validate_offline.py`
