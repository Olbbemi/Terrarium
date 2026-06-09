# toolshed/ — 공유 범용 인프라 (도메인-free)

키퍼의 도구창고. `orchard/`(앱 우산)의 형제 루트로, **도메인(Event/Todo/Goal)을 절대 모르는** 범용 코드만 둔다.

설계 근거: `orchard/CLI_Planning/code-design/02-design/handoff.md` 3장(모듈 구조)·6장(재사용성).

## 예정 구성 (아직 비어 있음 — 스켈레톤)

| 디렉토리 | 역할 | 채우는 단계 |
|---|---|---|
| `toolshed/log/` | Logger 인터페이스 + spdlog 구현 (구 rings) | A3 |
| `toolshed/sqlite/` | 연결/WAL/마이그레이션/RAII 래퍼 (구 roots의 범용 절반) | A4 |

## 규칙

- `toolshed`는 `orchard` 안을 **절대 import 하지 않는다** (단방향, 도메인-free 유지).
- 네임스페이스 루트는 `toolshed::` (`orchard`/앱과 형제 루트).
- 두 번째 소비자(형제 앱) 등장 시 상위로 `mv` 하여 추출 (재작성 아님). 현재는 Terrarium 내부 공유까지.

> A1 시점에는 디렉토리 골격만 존재. 최상위 CMake 의 `add_subdirectory(toolshed)` 는 타깃이 생기는 A3 에서 활성화.
