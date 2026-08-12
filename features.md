# 기능 — WAVEFORM v1.1.0

## 캡처

- Blackmagic **DeckLink** SDI 입력 (Desktop Video COM API)
- 장치 목록 / 선택
- **10-bit YUV (v210)** 우선, 실패 시 **8-bit UYVY**
- 입력 포맷 자동 감지 및 재설정
- 록 / 무신호 상태, 프레임 카운터
- 최신 프레임만 유지하는 저지연 큐
- **Color Bars 토글** — DeckLink가 있어도 강제 75% 컬러바
- 장치/신호 없을 때 시뮬레이터 자동 폴백

## 디스플레이

### Waveform
- Y / Cb / Cr 전압–시간 플롯
- **Parade** / **Overlay**
- **Line** / **Field** 스윕
- 성분 on/off (Y, Cb, Cr)
- 0 / 50 / 100% 스타일 계수선

### Vector
- Cb(X) vs Cr(Y)
- **75%** / **100%** 컬러바 타깃 박스
- 컴퍼스 / I–Q 축 오버레이

### Lightning
- 상단: **Pb vs Y**
- 하단: **Pr vs Y**
- 게인·크로마–루마 지연 확인용 타깃 마크

### Video (Picture)
- 동일 SDI/시뮬레이터 프레임의 실시간 화면
- BT.601 / BT.709 YCbCr→RGB (높이 기준 Auto)
- 화면비 유지 레터박스 / 필러박스
- Line Select 시 해당 라인 하이라이트
- Freeze는 스코프와 동일하게 적용

## 렌더링

- OpenGL 3.3 Core
- Y/Cb/Cr 텍스처 → 모드별 포인트 매핑
- R32F 가산 누적 + decay(persistence)
- Intensity / CRT 녹색 톤맵
- 타일 상태 문구는 QLabel로 표시 (잔상 방지)

## UI / 컨트롤

- 1 / 2 / 4 타일
- 타일별 Mode: Waveform / Vector / Lightning / Video
- Gain: 1x / 2x / 5x / Var
- Mag: 1x / 5x / 10x / 20x
- Line Select, Freeze
- Persistence / Intensity 슬라이더
- Color Bars 체크 시 Device 콤보 비활성화
- 상태바: 록/시뮬레이터/컬러바, push·drop·capture 카운터

## 도구

- CMake + Ninja + MSVC
- `tools/build.ps1`
- `tools/validate_offline.py` (v210 라운드트립, 벡터/라이트닝 검증)
