# 노트 — WAVEFORM v2.0.1

## 범위

v2.x는 **화상(픽처) 레이어** 분석에 집중하며, Vector/Lightning의 방송급 정확도(트레이스·타깃·계수선 정렬)를 강화했습니다.

포함:

- Waveform, Vector, Lightning, Video(Picture), None
- Color Bars(내장 시뮬레이터)
- 행렬 기반 타깃 / 연속 트레이스 / I–Q 참고 축 (HD 포함)

미포함 (의도적):

- Eye / Jitter (전용 PHY 하드웨어 영역, 역직렬화된 SDI만으로는 불가)
- Diamond / Arrowhead / Bowtie
- 오디오 미터 / 서라운드
- ANC / 자막 상세 디코드
- Composite 시뮬레이션

## 구조 (요약)

```
SDI → DeckLink 콜백 → FrameQueue → v210/UYVY 언팩 → Y/Cb/Cr
  → OpenGL 텍스처 → 모드별 셰이더
  → (스코프) 가산 누적 + decay + 톤맵
  → (Video) YCbCr→RGB 화면
  → Qt 계수선 + QLabel 상태 문구
```

| 경로 | 역할 |
|------|------|
| `src/capture/` | DeckLink 캡처 + 큐 + Color Bars |
| `src/video/` | v210 / UYVY 언팩 |
| `src/color/` | BT.601/709 행렬, Vector/Lightning 타깃 |
| `src/render/` | OpenGL 위젯 (포인트/선분 트레이스) |
| `src/display/` | 모드 / 계수선 (I/Q 참고축 항상) |
| `src/ui/` | 메인 창 / 타일 컨트롤 |
| `third_party/decklink/` | 설치된 `DeckLinkAPI64.dll` `#import` |

## 사용 팁

1. Desktop Video가 설치되어 `C:\Program Files\Blackmagic Design\Desktop Video\DeckLinkAPI64.dll` 이 있어야 합니다. 별도 SDK zip이 필수는 아닙니다.
2. `tools/build.ps1`의 Qt 경로는 기본 `C:\Qt\6.7.3\msvc2019_64` 입니다. 환경에 맞게 수정하세요.
3. 다른 PC로 exe만 옮길 때는 `windeployqt`로 배포된 DLL이 함께 있어야 합니다.
4. **Start** 후 상태바에 `LOCKED …` 또는 `COLOR BARS (forced) …` / `SIMULATOR …` 가 보이면 정상입니다.
5. 캡처카드 대신 컬러바만 보려면 상단 **Color Bars** 를 체크하세요.
6. Vector 타깃 정렬 확인 시 **Gain 1x**, Var Gain 해제. Gain은 트레이스만 확대/축소합니다.
7. I/Q 축은 NTSC 각도 기준 참고선이며, HD에서도 동일하게 표시됩니다 (컴포넌트 신호의 물리적 위상과 1:1은 아님).
8. 타임코드·문자·램프가 섞인 테스트 패턴은 꼭짓점 외에 중앙 잔여 트레이스가 보일 수 있습니다.

## 성능

- 목표: 1080i/p급, 전 샘플 누적(의도적 다운샘플 없음)
- Vector/Lightning 선분 연결은 라인 끝→다음 라인 시작 랩어라운드를 셰이더에서 차단
- 타일 수·persistence가 높으면 GPU 부하 증가 → 필요 시 타일 수 축소
- 큐는 최신 프레임만 유지해 지연을 제한

## 컬러메트리

- Auto: 높이 ≥720 → BT.709, 그 외 BT.601
- Vector / Lightning 타깃은 RGB→Y′CbCr 행렬로 계산 (75%/100% 바)
- I/Q는 NTSC 각도 기준 참고선으로 HD에서도 표시 (컴포넌트 신호의 물리적 위상과 1:1은 아님)
- 공인 legalizer / 교정기 대체품이 아닙니다

## 로컬 매뉴얼 PDF

설계 참고용 Tektronix PDF가 워크스페이스에 있을 수 있으나, 용량·저작권상 GitHub 릴리스에는 포함하지 않습니다.

## 알려진 제한

- Windows에서 DeckLink COM은 MSVC `#import`가 필요합니다.
- 내장 컬러바는 UI/경로 검증용이며, 송출망 교정 기준이 아닙니다.
- RGB Parade / Composite gamut 뷰는 이후 버전으로 미룹니다.
- 실 발생기의 SMPTE 바(플래시·램프·TC 오버레이)는 순수 풀프레임 바와 트레이스가 다를 수 있습니다.
