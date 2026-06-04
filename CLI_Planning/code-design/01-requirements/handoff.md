# 요구사항: CLI 기반 일정 관리툴 (Kyklos Gaia / CLI_Planning)

## 문제 정의
사용자(본인) 가 기존 일정 관리 도구의 일부 기능만 필요로 하고,
자기 워크플로우에 맞는 도구를 직접 만들어 점진적으로 확장하고자 한다.
최종 목표는 LLM/TTS/STT/GUI 어댑터를 붙여 사용자가 원하는 디자인의 통합 UI 를
완성하는 것이며, 본 CLI 단계는 그 기반(베이스)으로서 핵심 도메인 로직과
헥사고날 어댑터 구조를 검증·실사용하는 데 목적이 있다.

## 현재 상태
N/A (신규 프로젝트)

## 범위

### In scope
- 도메인 객체 3종: Event, Todo, Goal
- Event 시간 표현: 단일 시점/구간, 하루종일, 멀티데이, 단순 반복(매일/매주/매월/매년), 충돌 감지(인터랙티브 모드)
- Todo: 제목, 완료 상태, 우선순위(높음/중간/낮음), 태그, 마감일
- Goal: 이름, 목표값, 누적값(수기 입력 `goal log`), 기간(시작/종료일), 단위(자유 텍스트)
- Goal 진행률 ASCII 시각화 (`[######....] 60%`)
- 도메인 객체 관계: Goal 과 Todo 는 독립 (Goal 누적 ≠ Todo 완료)
- CLI 실행 시 진입 화면 자동 표시 (오늘 일정 / 마감 임박 할일)
- 로깅: 디버그 로그 + 감사 로그 (설정으로 on/off), 로그 레벨 조정 가능
- 데이터 영속화: SQLite (시스템 라이브러리)
- 운영 환경: Linux + macOS
- 구현: C++20, CMake + Ninja, in-tree vendoring (external/ 디렉토리)

### Out of scope
- 다중 사용자 / 공유 시나리오 (영구)
- Note (자유 메모)
- Todo 완료율 시각화, 시간 분배 비율 시각화 (Goal 진행률 외 모든 시각화)
- Goal 자동 리셋 주기 (필요시 추후 추가)
- Event 사용자 정의 반복 (매월 둘째 화요일 등) — GUI 단계로 위임
- Event 반복 일정 예외 처리 (특정 날 제외/변경) — GUI 단계로 위임
- 외부 캘린더 동기화 (Google/Apple/Outlook 등) — 영구
- iCalendar 형식 Import/Export
- OS 데스크탑 알림 / 백그라운드 데몬 — GUI 단계로 위임
- Windows 지원

### 보류
없음

## 비기능 요구사항
- 성능: p95 응답 시간 < 200ms (5년 누적 ~10K Event + ~5K Todo + ~100 Goal 가정)
- 보안 (인증/인가): N/A (로컬 1인, 외부 격리)
- 보안 (로컬 데이터): 평문 SQLite + OS 파일 권한 0600. 추후 SQLCipher 전환 가능
- 확장성 (사용자): 1인 영구. 다중 사용자 out
- 확장성 (아키텍처): 헥사고날 어댑터 추가 가능 (LLM/Voice/GUI/IPC). Use Case 는 JSON Schema 호환
- 가용성: N/A (1인 로컬 도구)
- 호환성: Linux (GCC 13+ 또는 Clang 16+) + macOS 13+. Windows out
- 운영 (로깅): 디버그 + 감사. 레벨 조정 + 감사 on/off 설정

## 제약 조건
- 언어: C++20
- 빌드 시스템: CMake + Ninja
- 의존성 관리: in-tree vendoring (external/) + OS 표준 라이브러리는 시스템 사용
  (적용: SQLite -> 시스템 / 그 외 -> in-tree)
- 데이터 저장: SQLite 단일 파일 DB
- STT/TTS/LLM 통합 (미래 어댑터 추가 시): 별도 프로세스 + 로컬 네트워크 통신
  (CLI 단계에는 IPC 구현 없음. 코어 설계가 미래 IPC 어댑터를 받쳐줄 수 있도록만 함 — NF3 참조)
- 라이선스 (본 도구): 미지정 (Public 공개하나 모든 권리 보유)
- 사용 라이브러리 라이선스: 자유 라이선스만 (SQLite=Public Domain, spdlog/fmt=MIT, gtest=BSD-3)
- 인력: 본인 1명
- 데드라인: 없음 (학습 + 본인 실사용)

## 우선순위
모든 in scope 요구사항이 필수. (전부 필수, 구현 순서는 2단계 설계에서 의존성 그래프로 결정)

## 요구사항별 승인 기준

| ID | 요구사항 | 승인 기준 |
|----|---------|----------|
| F1 | Event CRUD | `event add "회의" --start 2026-05-26T14:00 --end 15:00` 실행 -> DB 생성 + `event list` 포함. update/delete 필드 변경·제거 |
| F2 | Event 하루종일·멀티데이 | `--all-day --date ...` 시간 없는 Event. `--start ... --end ...` 멀티데이. list 적절 표시 |
| F3 | Event 단순 반복 | `--repeat weekly --until ...` 반복 Event. `event list --week <ISO주>` 인스턴스 포함 |
| F4 | Event 충돌 감지 | 기존 14:00~15:00 있을 때 새 14:30 시도 -> "충돌" 안내 + [추가/취소] 프롬프트 |
| F5 | Todo CRUD + 속성 | `todo add ... --priority --tags --due` 생성. `todo done <id>`. `todo list --today/--overdue/--tag/--priority` 필터 |
| F6 | Goal CRUD + 누적 | `goal add --target --unit --start --end` 생성. `goal log <name>` 누적 +1. `goal show` 현재/목표 표시 |
| F7 | Goal 진행률 시각화 | `goal show` 출력에 `[######....] 60%` 진행 바 |
| F8 | CLI 진입 자동 표시 | 인자 없는 실행 -> "오늘 일정 N개 / 마감 임박 Todo M개" 자동 출력 |
| F9 | 로깅 | 모든 명령 로그 파일 기록. 감사 on/off 설정. 로그 레벨 (DEBUG/INFO/WARN/ERROR) 설정 |
| NF1 | 응답 시간 | NF2 데이터 채워진 상태에서 모든 CLI 명령 p95 < 200ms (벤치마크 검증) |
| NF2 | 데이터 규모 | ~10K Event + ~5K Todo + ~100 Goal 삽입 상태에서 NF1 충족 |
| NF3 | 어댑터 확장 | 코어 변경 없이 신규 입력 어댑터 추가 가능. 어댑터 추가 테스트 통과 |
| NF4 | Use Case JSON Schema | 각 Use Case 입력/출력 타입에 대응 JSON Schema 정의 + 검증 통과 |
| NF5 | Linux + macOS | Ubuntu 22.04+ / macOS 13+ 양쪽에서 동일 빌드·실행 |
| NF6 | 빌드 환경 | GCC 13+ / Clang 16+ 로 `cmake -G Ninja && ninja` 한 번 빌드. external/ 외부 다운로드 0. SQLite 만 시스템 |
| NF7 | 파일 보호 | data.db 권한 0600. 외부 사용자 접근 차단 |

## 가정 사항
- 사용자는 Linux 환경을 메인으로 사용하고 macOS 호환을 원함 (사용자 확인됨)
- 사용자는 C++20 컴파일러(GCC 13+/Clang 16+)를 사용 가능 (사용자 확인됨)
- 사용자는 1인 본인 도구로 사용하며 외부 일정 시스템과의 연동을 영구 거부 (사용자 명시: "절대 사용하지 않음")
- Public Domain / MIT / BSD-3 라이선스 라이브러리만 사용 (자유 라이선스만 in-tree)

## 참고 자료
- 외부:
  - SQLite 공식 문서 (https://sqlite.org)
- 내부 wip 자료:
  - DDD 핵심 개념 — `CLI_Planning/knowledge/wip/domain-driven-design.md`
  - SQLite 정체와 운영 — `CLI_Planning/knowledge/wip/sqlite.md`

## 2단계 위임 항목
- D1: spdlog / fmt / gtest 구체 버전 선택 (in-tree vendoring)
- D2: 로그 파일 위치 / 회전 정책
- D3: SQLite 인덱스 / 테이블 스키마 설계 (NF1 달성 위함)
- D4: GUI 프레임워크 선택 (GUI 단계 진입 시)

## 사용자와의 명확화 기록
- 산출물 저장 경로를 본 프로젝트의 작업 디렉토리(`CLI_Planning/`) 하위로 사용자가 명시. 원칙(`{CWD}/code-design/...`) 대신 `CLI_Planning/code-design/01-requirements/` 사용.
- "의도(intent) 단위 설계" 의 의미를 사용자 확인하에 정의 — 코어가 사용자의 "하려는 일" 을 받도록 설계하는 패턴. CLI/GUI/Voice/LLM 어댑터가 자기 입력을 의도 객체로 변환.
- 도메인 객체 3종 선택 — Note 는 명시적 거부 ("쓸 일 없을 것"). Goal 은 Q2 운동 40회 같은 추적 객체로 정의 (Todo 와 독립).
- Goal 의 구조 — 카테고리 그룹이 아닌 별도 추적 객체 (B 구조: 목표값 + 누적값). 누적은 Todo 완료가 아닌 수기 명령(`goal log`).
- Event 시간 표현 범위 — 사용자 정의 반복 / 반복 일정 예외 처리는 CLI 단계에서 빼고 GUI 단계로 위임. 사용자 입장: "캘린더 세세한 기능은 안 필요" + 도메인 복잡도 비교 검토 후 결정.
- 충돌 감지 동작 모드 — "사용자 매번 선택" 인터랙티브 모드 채택. 사용자: "사용자한테 맡기는 구조로 가고 나중에 변경 가능".
- 외부 캘린더 동기화 — 사용자 명시 강한 거절 ("절대 사용하지 않음"). 영구 out 으로 기록.
- 알림 정책 — CLI 실행 시 자동 표시만. OS 데스크탑 알림 / 백그라운드 데몬은 GUI 단계로 위임.
- 시각화 라이브러리 — 사용자 결정으로 CLI 단계에서는 외부 라이브러리 미사용. C++ GUI 라이브러리(Qt 등) 의 가능성만 확인하고 GUI 단계 위임.
- 구현 언어 — C++. STT/TTS/LLM 같이 C++ 직접 통합이 어려운 시스템은 별도 프로세스로 분리하고 로컬 네트워크 통신.
- 빌드 시스템 — CMake + Ninja (Linux + macOS 양쪽 호환).
- 의존성 관리 정책 — 사용자 정의 원칙: "OS 표준 라이브러리는 시스템 사용, 그 외는 in-tree vendoring (external/)". 적용 1: SQLite -> 시스템. 적용 2 (잠정): spdlog/fmt/gtest -> in-tree.
- 라이선스 — Public 공개하나 명시 안 함. 사용자: "공개해도 어차피 아무도 안 씀". view-only 상태가 의도된 것임을 사용자 명시 후 확정.
- 2단계 위임 항목에서 D2 (IPC 라이브러리 선택) 제거. CLI 단계 자체에는 IPC 통신이 없으며, IPC 어댑터는 미래 단계(LLM/Voice 어댑터 추가 시) 결정 사항. NF3 (어댑터 추가 가능 구조)는 CLI 코어 설계의 비기능 요구사항으로 유지.
