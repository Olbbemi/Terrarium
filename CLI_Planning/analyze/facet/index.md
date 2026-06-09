# Terrarium CLI_Planning — as-built 코드 지도 인덱스

C++ 헥사고날(포트-어댑터) CLI 도구. 일정(Event)·할일(Todo)·목표(Goal) 세 도메인을 SQLite 에 저장하고 관리한다. 식물 메타포 네이밍을 쓴다.

## 진입점
- 실행 진입점: `src/glass/main.cpp` (`main`) — 유일한 Composition Root. 빌드 산출물 `gaia`.
- 논리 진입점: `src/stem/*UseCase::execute()` 16개 (Event/Todo/Goal 의 CRUD·List·Show + Dashboard).

## 레이어 (식물 메타포)
| 디렉토리 | 레이어 | 빌드 타겟 |
|---|---|---|
| `glass` | 진입점/조립 | `gaia` (exe) |
| `stem` (+`commands`) | 유스케이스 + 입력 DTO | `planning_core` |
| `seed` | 도메인 엔티티/값객체/서비스 | `planning_core` |
| `stomata` | 포트(추상 인터페이스) | `planning_core` |
| `roots` | SQLite 저장소 어댑터 | `planning_roots` |
| `rings` | spdlog 로깅 어댑터 | `planning_rings` |
| `climate` | toml++ 설정 어댑터 | `planning_climate` |
| `leaves` | CLI 포맷/충돌 프롬프트 어댑터 | `planning_leaves` |

## facet 목차 (어디에 무엇이 있나)
- **architecture.md** — 모듈/디렉토리 레이아웃, 빌드 타겟 대응, 의존성 방향(의존성 역전·순환 없음), 진입점. 모듈 의존 flowchart.
- **types.md** — 도메인 엔티티/값객체/열거형, 포트 인터페이스, 어댑터 구현체, 16개 유스케이스의 입력 DTO·포트 의존. 클래스 다이어그램 3종(포트 구현 / 도메인 합성 / 유스케이스 의존).
- **flow.md** — 주요 3시나리오 호출/데이터 흐름: (1) `event add` 충돌검사·프롬프트 쓰기경로, (2) 무인자 대시보드(두 저장소 동시 조회), (3) `init` 설정 부트스트랩(Composition Root 이전 분기). 시퀀스 다이어그램은 HTML 산출물 참조.
- **externals.md** — 벤더 라이브러리 6종(CLI11/SQLiteCpp/spdlog/stduuid/toml++/googletest)과 버전·사용 위치. nutrients/ 에 in-tree 벤더링.
- **summary.md** — 모듈별 동작을 사람 말로 서술. 관통 원칙: 시간/타임존/달력 계산은 엣지(glass/leaves)에서, 내부는 UTC instant·달력날짜만. 충돌검사·반복 이벤트 전개·목표 누적 등 로직 핵심.
- **callgraph-cpp.md** — clang/LLVM 함수 호출 그래프, 큐레이션본(의미함수 84개 — ctor/dtor·getter·익명ns 헬퍼·람다 제외, 진입점 17). 헥사고날 경계의 정적 증거(stem->어댑터 직접엣지 0, 전부 포트 경유 추정). HTML 에 모듈 오버뷰 + full-clustered + 진입점별 드릴다운 17장 SVG 포함.

## 산출물
- `facet/` — 위 6개 facet (Claude 용 영구 지도).
- `html/index.html` — 인터랙티브 시각화(Mermaid 다이어그램 + 호출그래프 SVG).
- `markdown/ARCHITECTURE.md` — as-built 마크다운 문서.
