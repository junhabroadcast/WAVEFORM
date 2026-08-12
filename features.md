# 기능 — WAVEFORM v2.0.1

## 캡처

- Blackmagic **DeckLink** SDI 입력 (Desktop Video COM API)
- 장치 목록 / 선택
- **10-bit YUV (v210)** 우선, 실패 시 **8-bit UYVY**
- 입력 포맷 자동 감지 및 재설정
- 록 / 무신호 상태, 프레임 카운터
- 최신 프레임만 유지하는 저지연 큐
- **Color Bars 토글** — DeckLink가 있어도 강제 75% 컬러바
- 장치/신호 없을 때 시뮬레이터 자동 폴백
- 캡처 재시작 시 이전 포맷명(SIM 등)이 남지 않도록 초기화

## 디스플레이

### Waveform
- Y / Cb / Cr 전압–시간 플롯
- **Parade** / **Overlay**
- **Line** / **Field** 스윕
- 성분 on/off (Y, Cb, Cr)
- 0 / 50 / 100% 스타일 계수선

### Vector
- Cb(X) vs Cr(Y)
- **연속 트레이스** — 인접 샘플을 `GL_LINES`로 연결 (컬러바 육각형 궤적)
- **75%** / **100%** 타깃 박스 — BT.601 / BT.709 행렬로 정확한 위치 계산
- 트레이스와 계수선 **동일 중심·스케일** (박스에 꼭짓점이 들어감)
- **I/Q 축** — Q=33° / I=303° 라벨 포함 (HD에서도 참고용으로 표시)

### Lightning
- 상단: **Pb vs Y**
- 하단: **Pr vs Y**
- Vector와 동일한 연속 트레이스 + 행렬 기반 타깃(W + 6색)
- 계수선과 좌표 정렬

### Video (Picture)
- 동일 SDI/시뮬레이터 프레임의 실시간 화면
- BT.601 / BT.709 YCbCr→RGB (높이 기준 Auto)
- 화면비 유지 레터박스 / 필러박스
- Line Select 시 해당 라인 하이라이트
- Freeze는 스코프와 동일하게 적용

### None
- 타일을 검정 빈 화면으로 비움 (스코프/비디오 렌더 없음)

## 렌더링

- OpenGL 3.3 Core
- Y/Cb/Cr 텍스처 → 모드별 매핑
- Waveform: 포인트 / Vector·Lightning: 선분 + 포인트
- R32F 가산 누적 + decay(persistence), **기본 persistence = 0**
- Intensity / CRT 녹색 톤맵
- 타일 상태 문구는 QLabel로 표시 (잔상 방지)
- 모드 전환 시 누적 버퍼 초기화

## UI / 컨트롤

- 1 / 2 / 4 타일
- 타일별 Mode: Waveform / Vector / Lightning / Video / **None**
- Gain: 1x / 2x / 5x / Var
- Mag: 1x / 5x / 10x / 20x
- Line Select, Freeze
- Persistence (0–99, 기본 0) / Intensity 슬라이더
- Color Bars 체크 시 Device 콤보 비활성화
- 상태바: 록/시뮬레이터/컬러바, push·drop·capture 카운터

## 도구

- CMake + Ninja + MSVC
- `tools/build.ps1`
- `tools/validate_offline.py` (v210 라운드트립, 벡터/라이트닝 검증)
