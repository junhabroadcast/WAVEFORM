# Changelog

**WAVEFORM**의 주요 변경 사항을 기록합니다.

## [2.0.0] — 2026-08-12

### 추가

- **Vector / Lightning 연속 트레이스** — 인접 샘플을 선분으로 연결해 CRT형 궤적 표시
- Vector/Lightning 포인트 크기 확대 (타깃 지점 가시성 향상)
- **행렬 기반 타깃** — BT.601 / BT.709로 75%/100% 컬러바 박스 정확 계산
- Lightning 타깃에 W + 6색 라벨
- **I/Q 축 라벨** (Q=33°, I=303°) — BT.601(SD)에서만 표시

### 수정

- 트레이스와 계수선 **좌표 불일치** 수정 (중심·스케일 공유 → 박스가 신호에 맞음)
- Lightning x축 매핑 오류 수정
- HD에서 의미 없는 I/Q 축 제거 (BT.709에서는 숨김)
- 시뮬레이터 → 실 SDI 전환 시 상태 문구에 `SIM …` 잔존하던 문제 수정

### 문서

- README / features / notes / changelog 를 **v2.0.0** 기준으로 갱신

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
