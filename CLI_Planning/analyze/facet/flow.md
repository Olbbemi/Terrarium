# Facet 3: 플로우 — 주요 시나리오의 호출/데이터 흐름

이 CLI 는 단일 진입점 `main`(glass) 에서 CLI11 로 인자를 파싱하고, "Composition Root" 에서 모든 어댑터·도메인·유스케이스를 한 번에 조립한 뒤, 어느 서브커맨드가 파싱됐는지에 따라 해당 유스케이스(stem)를 호출하는 헥사고날 구조다. 유스케이스는 포트 인터페이스(stomata)를 통해서만 저장소(roots, SQLite)와 도메인(seed)에 접근하고, 결과를 main 으로 돌려주면 main 이 leaves 의 포맷 함수로 콘솔에 출력한다. 아래 세 시나리오는 (1) 충돌 검사·사용자 프롬프트라는 가장 분기가 많은 쓰기 경로, (2) 서브커맨드 없이 들어오는 기본 경로이자 두 저장소를 동시에 읽는 조회 경로, (3) Composition Root 진입 전에 따로 처리되는 예외적 부트스트랩 경로를 각각 대표하도록 골랐다.

## 공통 부트스트랩

`main` 은 먼저 `--config` 등 모든 옵션과 서브커맨드를 CLI11 에 등록하고 `CLI11_PARSE` 로 파싱한다. `init` 이 파싱된 경우에만 Composition Root 진입 전에 분기해 처리하고(시나리오 3), 그 외에는 try 블록 안에서 `TomlConfigLoader` 로 설정을 읽고, `SpdlogLogger`·`SQLite::Database`(WAL/foreign_keys PRAGMA 설정)·`MigrationRunner.run` 으로 스키마를 올린 뒤, 세 SQLite 저장소(event/todo/goal)·`ConflictDetector`·`StdUuidGenerator`·`CliConflictPrompter` 를 만들고, 이들을 생성자 주입해 16개 유스케이스를 전부 인스턴스화한다. 그 다음 `eventAdd->parsed()` 부터 시작하는 거대한 if/else 체인이 실제 디스패치를 담당한다. 어느 분기에서든 예외가 던져지면 맨 바깥 catch 가 `오류: ...` 를 stderr 로 찍고 종료코드 1 을 반환한다.

## 시나리오 1: `event add` — 충돌 검사와 사용자 확인이 있는 쓰기 경로

대표성: 저장 전에 도메인 검증·저장소 조회·도메인 충돌 판정·대화형 프롬프트를 모두 거치는, 가장 흐름이 긴 쓰기 경로다.

`main` 의 `eventAdd` 분기는 파싱된 문자열 옵션을 leaves 의 `parseDateTime`/`makeRule`(`--repeat`/`--until` 을 `RecurrenceRule` 로 변환) 로 변환해 `CreateEventCommand` 를 채우고 `createEvent.execute(cmd)` 를 호출한다. `CreateEventUseCase::execute` 는 먼저 `domain::TimeRange` 생성자로 시간 구간을 검증한다(잘못된 구간이면 여기서 `invalid_argument` 가 올라가 main 의 catch 로 빠진다). 이어 `events_.findOverlapping(range)` 로 겹칠 수 있는 후보 이벤트들을 조회하는데, SQLite 저장소는 `start_ts < candEnd AND COALESCE(end_ts, start_ts) > candStart` 조건의 SELECT 로 후보 행을 모아 도메인 `Event` 로 복원한다. 유스케이스는 그 후보 목록을 `detector_.detect(range, overlapping)` 에 넘기고, `ConflictDetector` 는 각 후보의 `TimeRange::overlaps` 를 실제로 확인해 첫 충돌을 `Conflict`(기존 id·제목·시간) 로 반환한다. 충돌이 있으면 `prompter_.promptOnConflict` 가 호출되어 `CliConflictPrompter` 가 "...겹칩니다. 그래도 추가할까요? [y/N]" 를 stdout 에 출력하고 stdin 에서 한 줄을 읽는다. 입력이 `y`/`Y`/`yes` 가 아니거나 EOF 이면 `CANCEL` 을 반환하고, 유스케이스는 event.create 감사 로그를 남긴 뒤 `cancelledByUser=true` 인 Result 를 돌려줘 main 이 "취소되었습니다" 를 출력한다. 진행으로 답하면 `idGen_.next()` 로 UUID 를 발급해 `domain::Event` 를 만들고 `events_.save(event)` 가 INSERT 를 실행하며, 감사 로그를 남기고 Result 를 돌려줘 main 이 "Event ... 추가 완료" 를 출력한다.

## 시나리오 2: 서브커맨드 없음 — 대시보드(두 저장소 동시 조회)

대표성: 서브커맨드를 주지 않았을 때 도달하는 기본 경로이자, 한 유스케이스가 두 개의 다른 저장소를 함께 읽어 집계하는 유일한 조회 경로다.

if/else 체인의 모든 `parsed()` 가 거짓이면 최종 else 가 실행된다. `main` 은 `localTodayDate()` 로 시스템 로컬 타임존 기준 오늘 달력 날짜를 구하고, 이를 `ShowDashboardQuery` 에 담는다. `todayDate` 는 todo 의 기한초과 비교용 로컬 달력 날짜로, `dayStart`/`dayEnd` 는 로컬 [오늘 자정, 내일 자정) 을 `localMidnightUtc` 로 UTC instant 로 변환한 값으로 채워 event 범위 조회에 쓴다. `dashboard.execute(q)` 안에서 `ShowDashboardUseCase` 는 `events_.findInRange(dayStart, dayEnd)` 로 오늘 범위에 걸치는 이벤트들을 조회하고(SQLite 가 findOverlapping 과 같은 범위 SELECT 사용), 이어 `todos_.findOverdue(todayDate)` 로 기한이 지난 미완료 todo 들을 조회한다. 두 결과의 `size()` 만 추려 `Result{todayEventsCount, overdueTodosCount}` 로 반환하고(반복 이벤트 인스턴스 카운트는 미구현 TODO), 감사 로그 `dashboard.show` 를 남긴다. main 은 "오늘 일정 N개 / 기한 초과 Todo M개" 한 줄을 출력한다.

## 시나리오 3: `init` — Composition Root 이전의 설정 부트스트랩

대표성: 설정을 "읽는" 다른 모든 명령과 달리 설정을 "쓰는" 명령이라, DB·로거·저장소 조립(Composition Root) 자체를 건너뛰고 파싱 직후 따로 처리되는 예외적 흐름이다.

`CLI11_PARSE` 직후 `main` 은 `initCmd->parsed()` 를 가장 먼저 검사한다. 참이면 `--db`/`--log` 경로를 `renderDefaultConfig(initDb, initLog)` 에 넘기는데, climate 어댑터는 toml++ 로 `[database].path` 와 `[log]` 섹션(level=INFO, audit=true, rotation=none, retention 일수 등 기본값)을 가진 테이블을 만들어 TOML 문자열로 직렬화해 반환한다. main 은 그 문자열을 `--config` 로 지정된 경로에 `trunc`(항상 덮어쓰기) 모드 `ofstream` 으로 기록하고, 파일을 열 수 없으면 `runtime_error` 를 던진다(로컬 try/catch 가 받아 "오류: ..." 를 stderr 로 출력하고 1 반환). 성공하면 "설정 파일 생성: ..." 를 출력하고 즉시 `return 0` 하므로 SQLite 연결이나 마이그레이션은 전혀 일어나지 않는다.

---

분석에 사용한 핵심 파일:
- `src/glass/main.cpp` (디스패치·Composition Root)
- `src/stem/CreateEventUseCase.cpp`
- `src/stem/ShowDashboardUseCase.cpp`
- `src/roots/SqliteEventRepository.cpp`
- `src/seed/ConflictDetector.cpp`
- `src/leaves/CliConflictPrompter.cpp`
- `src/climate/DefaultConfig.cpp`
