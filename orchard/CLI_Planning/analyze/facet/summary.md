# 로직요약 (facet 5) — Terrarium CLI 일정/할일/목표 관리 도구

이 프로젝트는 **이벤트(일정), 할일(Todo), 목표(Goal)** 세 도메인을 다루는 CLI 도구다. 헥사고날(포트/어댑터) 구조로, 식물 메타포 네이밍을 쓴다: `glass`(진입점), `stem`(유스케이스), `seed`(도메인), `stomata`(포트 인터페이스), `roots`(SQLite 어댑터), `climate`(설정), `rings`(로깅), `leaves`(CLI 입출력 어댑터). 일관되게 관통하는 설계 원칙은 **"벽시계·로컬 타임존·달력 계산은 모두 엣지(glass/leaves)에서 하고, 내부 도메인/저장소는 UTC instant(`sys_seconds`)와 순수 달력 날짜(`sys_days`)만 다룬다"**는 것이다.

## glass — 진입점 / Composition Root (`src/glass/main.cpp`)

프로그램의 유일한 `main`. CLI11로 서브커맨드 트리(`event/todo/goal` 각각의 add·list·update·delete 등과 `init`, 무인자 시 대시보드)를 정의하고, 파싱한 인자를 각 유스케이스의 Command/Query 객체로 변환해 호출한다. 책임은 세 가지다.

- **의존성 조립(Composition Root)**: 설정 로드 -> 로거 생성 -> SQLite DB 연결 -> 마이그레이션 실행 -> 저장소·도메인 서비스·유스케이스를 전부 손으로 생성·주입한다. 여기가 구체 구현(SQLite, spdlog, toml, 표준 UUID)을 아는 유일한 자리다.
- **벽시계 의존 정책**: `localTodayDate()`로 시스템 로컬 타임존 기준 "오늘"을 읽는다. 의도적으로 Composition Root에만 둔 부수 효과(시스템 시계 읽기)다.
- **부분 수정 플래그 충돌 검증**: `--end`/`--clear-end`, `--all-day`/`--no-all-day`, `--repeat`/`--no-repeat`, `--due`/`--clear-due`, `--tag`/`--clear-tags`처럼 상호 배타적인 플래그가 동시에 오면 거부한다. 목표 기간 변경은 `--from`/`--to`를 반드시 함께 줘야 한다.

부수 효과: 표준출력/표준에러로 결과·오류 출력, 모든 도메인 예외를 잡아 "오류: ..." 출력 후 종료코드 1 반환. **`init` 명령은 설정 파일을 쓰는 동작이라, 설정을 읽는 Composition Root 진입 전에 따로 처리**한다(설정 파일을 항상 덮어쓰기 `trunc`로 생성).

가정/제약: `--config`는 항상 필수. 충돌 프롬프트 응답은 `std::cin`에서 읽는다(대화형 전제).

## seed — 도메인 엔티티와 순수 규칙 (`src/seed/`)

외부 의존이 없는 핵심 비즈니스 객체들. 모두 생성자에서 불변식을 검증한다.

- **Event**: 일정 애그리거트. 제목·시간구간(`TimeRange`)·선택적 반복규칙을 가진다. 빈 제목이면 예외.
- **Todo**: 할일 애그리거트. 제목·우선순위·태그·마감일·완료여부. 태그 추가는 멱등(이미 있으면 무시).
- **Goal**: 목표 애그리거트. 목표값 대비 누적값(`currentValue`)으로 달성률을 계산. `targetValue<=0` 또는 `periodStart>periodEnd`면 예외. 달성률은 상한이 없어 초과 달성 시 1.0을 넘을 수 있다. 영속성 복원용으로 누적값을 받는 별도 생성자를 둔다.
- **TimeRange**: 시작·(선택)종료·종일 여부 값 객체. `start>end`면 예외. 핵심은 `overlaps()` — **종료시각이 없으면 시작시각을 종료로 간주**(`effectiveEnd`)하고 반열린구간 겹침 판정을 한다. 충돌 탐지의 근거.
- **ConflictDetector**: 후보 시간구간과 "겹치는 후보 목록"을 받아 첫 충돌을 반환하는 순수 서비스. 실제 겹침 후보 조회는 저장소가 하고, 이 클래스는 판정만 한다.
- **StdUuidGenerator**: `random_device` 시드로 UUID v4 생성(부수 효과: 엔트로피 소비). RecurrenceRule/Priority는 값 타입 정의.

## stem — 유스케이스 (애플리케이션 서비스, `src/stem/`)

각 유스케이스는 포트(저장소/로거/프롬프터)만 의존하고, 도메인 객체를 조립·조작해 저장소에 위임한다. 공통 패턴: 실행 끝에 `logger.audit(...)`로 감사 로그를 남긴다. 대부분 단순 CRUD 위임이고, 로직이 실린 곳은 다음이다.

- **CreateEventUseCase / UpdateEventUseCase**: 저장 전에 **시간 충돌을 검사**한다. 저장소의 `findOverlapping`으로 겹치는 이벤트를 모으고, `ConflictDetector`로 판정한 뒤, 충돌이 있으면 `ConflictPrompter`로 사용자에게 물어 강행/취소를 결정한다. Update는 자기 자신을 충돌 대상에서 제외하고, 시간이 바뀐 경우에만 충돌 검사를 한다. 취소 시 저장하지 않고 `cancelledByUser`를 반환.
- **ListEventsUseCase**: 가장 로직이 무거운 곳. `findAll`로 전부 가져온 뒤(주석에 인덱스 최적화 TODO 명시), **비반복 이벤트는 윈도우와 겹침 필터**, **반복 이벤트는 `[winStart,winEnd)` 안의 occurrence들로 전개**한다. 전개는 빈도(daily/weekly/monthly/yearly)만큼 시작시각을 전진시키며, 월/년 반복에서 존재하지 않는 날(예: 1/31->2월)은 그 달 마지막 날로 클램프한다. 무한루프 방지용 `kSafetyCap=200000` 상한이 있고, `until`을 넘거나 윈도우를 벗어나면 멈춘다. 결과는 시작시각 오름차순 정렬.
- **CreateGoalUseCase**: 같은 이름의 목표가 이미 있으면 거부(이름 유일성 보장).
- **LogGoalUseCase / ShowGoalUseCase**: 이름으로 직접 조회(`findByName`)해 진행을 +1 하거나 달성률을 보여준다. 진행 기록은 **기간 종료 여부와 무관하게 누적**된다(의도적으로 기간 만료를 막지 않음).
- **ListTodosUseCase**: 전부 가져와 인메모리로 `오늘 마감/기한초과/태그/우선순위` 필터를 AND 결합. "기한초과"는 미완료이면서 마감일이 기준일보다 과거인 것만.
- **UpdateTodoUseCase**: 부분 수정. 태그가 주어지면 기존 태그를 전부 제거 후 새 목록으로 전체 교체.
- **ShowDashboardUseCase**: 오늘의 이벤트 수와 기한초과 Todo 수를 집계. 주석에 "반복 인스턴스 카운트는 미반영(TODO)" 명시 — 즉 대시보드의 이벤트 카운트는 비반복 기준이다.

가정/제약: Update/Delete/Done/Log 계열은 대상이 없으면 `std::out_of_range`를 던진다(엣지에서 잡아 오류 출력).

## stomata — 포트 인터페이스 (`src/stomata/`)

순수 추상 인터페이스 모음: `EventRepository`, `TodoRepository`, `GoalRepository`, `Logger`, `ConfigLoader`, `ConflictPrompter`. 유스케이스가 구체 구현 대신 이것들에 의존하게 해 테스트 대역 주입과 어댑터 교체를 가능하게 한다. 저장소 포트는 "로컬 달력 경계 계산은 엣지가 하고 UTC instant로 넘긴다"는 계약을 주석으로 못박는다.

## roots — SQLite 영속성 어댑터 (`src/roots/`)

포트 구현체로 도메인 객체 <-> DB 행을 매핑한다(부수 효과: DB 읽기/쓰기).

- **SqliteEventRepository**: 시각은 epoch 초(int64)로, UUID는 문자열로 저장. 핵심 로직은 `findOverlapping`/`findInRange`의 겹침 SQL — `start_ts < ? AND COALESCE(end_ts, start_ts) > ?`로 도메인의 반열린구간 겹침 규칙을 그대로 구현. 반복규칙은 `recur_freq`/`recur_until` 컬럼에 저장.
- **SqliteGoalRepository**: 날짜는 epoch days로 저장. `findByName`으로 이름 조회 지원(Create의 유일성 검사, Log/Show의 직접 조회 근거).
- **SqliteTodoRepository**: (동형) Todo CRUD.
- **MigrationRunner**: `schema_version` 테이블을 만들고, 마이그레이션 디렉토리(`TERRARIUM_MIGRATIONS_DIR`)에서 `.sql` 파일을 파일명 앞 숫자(버전) 순으로 정렬·적용한다. 이미 적용된 버전은 건너뛰고, 각 마이그레이션을 트랜잭션으로 감싸 실패 시 롤백한다(부수 효과: 파일 읽기 + DB DDL).

## climate — 설정 (`src/climate/`)

- **TomlConfigLoader**: TOML 설정 파일을 읽어 DB 경로(필수)·로그 설정(경로 필수, 레벨/감사/회전/보존일수 등은 기본값)을 제공. 파일 없음과 파싱 오류를 모두 `runtime_error`로 통일(부수 효과: 파일 읽기).
- **DefaultConfig (`renderDefaultConfig`)**: `init` 명령이 쓸 기본 설정 TOML 문자열을 생성. toml++가 키를 정렬 직렬화하므로 출력 순서는 비보장이라고 주석으로 명시.

## rings — 로깅 (`src/rings/SpdlogLogger.cpp`)

spdlog 기반 Logger 포트 구현. 설정에 따라 회전 전략(none/daily/size)을 골라 파일 싱크를 만들고, 일반 디버그 로그와 감사 로그를 분리(`separateDebugAudit`)할 수 있다. **파일 싱크 생성에 실패하면 예외를 밖으로 던지지 않고 stderr로 폴백** — 로깅 실패가 앱을 죽이지 않게 한다. `audit`가 꺼져 있으면 감사 로그를 무시(부수 효과: 파일/표준에러 쓰기).

## leaves — CLI 입출력 어댑터 (`src/leaves/`)

테스트 가능하도록 main에서 분리한 순수 변환·입출력 계층.

- **CliFormat**: 문자열 파싱(`parseDateTime`/`parseDate`/`parsePriority`/`parseFrequency`)과 포맷팅(`formatDate`/`formatDateTime`/`progressBar`/각종 텍스트화), 그리고 **로컬<->UTC 변환의 핵심**(`localCivilToUtc`/`localMidnightUtc`). 사용자가 입력하는 로컬 시각을 타임존으로 해석해 UTC instant로 바꾸고, 저장된 UTC instant를 다시 로컬로 표시한다. 잘못된 형식은 예외. 진행 막대는 `#`/`-` ASCII로 그린다.
- **CliConflictPrompter**: 충돌 시 "[y/N]"로 묻고 응답을 읽는다. **`y/Y/yes`만 강행, 그 외와 EOF(비대화형 입력)는 안전하게 취소**(부수 효과: 표준입출력).

## 전체적 가정과 제약

- 시간 정책: 사용자 입력/표시는 시스템 로컬 타임존, 내부 저장은 UTC. "오늘/이번 주" 같은 달력 경계는 엣지에서 계산해 UTC instant로 변환 후 저장소에 넘긴다.
- 목록/대시보드의 반복 이벤트 처리는 인메모리 `findAll` 후 전개(성능 최적화는 TODO로 남김), 대시보드 이벤트 카운트는 반복 인스턴스를 세지 않는다.
- 목표 이름은 유일하며, 목표 진행 기록은 기간 만료를 검사하지 않고 무조건 누적한다.
- 비대화형 환경에서 충돌이 나면 입력이 없으므로 항상 취소된다.

---

관련 소스 경로:
- `src/glass/main.cpp`
- `src/stem/ListEventsUseCase.cpp` (반복 전개 로직)
- `src/stem/CreateEventUseCase.cpp`, `UpdateEventUseCase.cpp` (충돌 처리)
- `src/seed/TimeRange.cpp`, `ConflictDetector.cpp`, `Goal.cpp`
- `src/roots/MigrationRunner.cpp`, `SqliteEventRepository.cpp`
- `src/leaves/CliFormat.cpp`, `CliConflictPrompter.cpp`
- `src/rings/SpdlogLogger.cpp`, `src/climate/TomlConfigLoader.cpp`
