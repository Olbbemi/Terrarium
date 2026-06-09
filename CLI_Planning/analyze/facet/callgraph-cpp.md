# C++ 함수 호출 그래프 (facet: callgraph-cpp)

> **이 facet 은 큐레이션본이다.** clang/LLVM `dot-callgraph` 원본에서 프로젝트 `planning::` 함수만 남기고 ctor/dtor·사소한 getter(IR 실체 기준)·익명 네임스페이스(파일-로컬) 헬퍼·람다 클로저를 제거했다. 최종 **의미함수 84개**만 다룬다(원본 전체 그래프가 아니다). stomata(ports)는 순수 추상 인터페이스라 정적 호출그래프에 함수 노드가 0개이며, 모듈 박스는 "포트 seam" 표현용으로만 유지한다. 간접/가상 호출 엣지는 정적 단정 불가라 클래스계층 근사(추정)이며 점선+"est" 라벨이다.

## 추출 메타
- 도구: clang/LLVM 17. 각 TU `clang++ -emit-llvm`(+`--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/13` 로 C++20 `time_zone` 헤더 함정 우회) -> `llvm-link` -> `opt -passes=dot-callgraph` -> `llvm-cxxfilt` 디맹글.
- 대상: 프로젝트 `src/` 32개 TU. 벤더(nutrients/)·테스트(observation/)·build/ 제외. 모듈 소속은 디맹글 네임스페이스(함수 한정자)로 판정.
- 노드 ID 는 맹글 심볼 전체 해시(꼬리 자르기 금지 — std::string 인자 함수 충돌 방지). 고유함수수 == 고유노드ID수.

## 아키텍처 한눈에 (헥사고날)
- 진입점은 `main`(glass) 하나. main 이 climate 로 설정을 읽고(`TomlConfigLoader::dbPath/logConfig`, 없으면 `renderDefaultConfig`), roots 의 `MigrationRunner::run` 으로 DB 마이그레이션을 돌린 뒤, 16개 유스케이스(stem)를 직접 디스패치한다.
- 의존 방향: glass -> stem(application) -> seed(domain) 은 **직접 호출**. stem -> {roots, rings, leaves} 는 stomata(ports) 인터페이스를 통한 **가상 디스패치(추정)**. 도메인(seed)은 어느 어댑터도 호출하지 않음(의존 역전 성립).

## 진입점 목록 (17개)
- `main` (glass / CLI entrypoint) — 유일 진입점. depth-1 로 16개 `*UseCase::execute` 와 CLI 포맷 헬퍼(leaves), config(climate), 마이그레이션(roots)을 호출.
- 유스케이스 진입점 16개(stem): AddTodo, CreateEvent, CreateGoal, DeleteEvent, DeleteGoal, DeleteTodo, ListEvents, ListGoals, ListTodos, LogGoal, MarkTodoDone, ShowDashboard, ShowGoal, UpdateEvent, UpdateGoal, UpdateTodo.

## 모듈 간 호출 관계 (overview-modules 라벨 근거)
- 직접 호출(실선): glass->stem 16, glass->seed 9, glass->leaves 9, glass->climate 3, glass->roots 1; stem->seed 20; roots->seed 14; seed->seed 3; leaves->leaves 2.
- 추정 호출(점선, 포트 경유): stem->roots 17, stem->rings 16, stem->leaves 2.
- 핵심 관찰: stem 은 roots/rings 를 **직접 호출하지 않는다** — 전부 추정(포트 경계). 이것이 포트-어댑터 의존성 역전이 정적 그래프에 남긴 흔적이다.

## 핵심 호출 체인
- 쓰기 흐름(이벤트 생성): `main -> CreateEventUseCase::execute -> ConflictDetector::detect`(seed 직접) -> [추정] ConflictPrompter(leaves) 충돌 확인 -> [추정] EventRepository(roots) 저장 -> [추정] Logger(rings) 기록.
- 충돌 검사 내부: `ConflictDetector::detect -> TimeRange::overlaps -> TimeRange::effectiveEnd` (seed 내부 체인).
- 갱신 흐름(도메인 풍부): `UpdateEventUseCase::execute` 가 seed 도메인 메서드를 직접 다수 호출(`Event::reschedule/rename/setRecurrence`, `ConflictDetector::detect`). `UpdateGoalUseCase`->`Goal::updatePeriod/updateTarget/rename`, `UpdateTodoUseCase`->`Todo::addTag/removeTag/rename/setDueDate`.
- 영속화->도메인 읽기: roots 리포지토리가 seed 접근자를 직접 호출. `SqliteEventRepository::update -> {Event::id, Event::recurrenceRule, ...}`, `SqliteGoalRepository::save/update -> Goal::id/periodStart/periodEnd`.
- 부트스트랩: `main -> TomlConfigLoader::dbPath/logConfig`(없으면 `renderDefaultConfig`) -> `MigrationRunner::run`.

## 추정(estimated) 엣지가 끼는 지점
정적 단정 불가한 가상 디스패치는 전부 stem 유스케이스 `execute` -> stomata 포트 -> 구현 모듈 구간이다(점선 "est: <Interface>"). 매핑:
- `*Repository` 포트 -> roots `Sqlite{Event,Todo,Goal}Repository` (17 엣지)
- `Logger` 포트 -> rings `SpdlogLogger` (16 엣지, 거의 모든 유스케이스 보유)
- `ConflictPrompter` 포트 -> leaves `CliConflictPrompter` (2 엣지: CreateEvent, UpdateEvent 만)
- `ConfigLoader` 포트 -> climate `TomlConfigLoader` (main 에서 구체타입 와이어링이라 drilldown-main 에선 실선)

## 시각화 그래프 (사람용 SVG, HTML 산출물 참조)
- `overview-modules`: 8모듈 노드 + 직접/추정 호출건수 라벨.
- `full-clustered`: 모듈별 `subgraph cluster_` 박스(조밀 배치)에 84개 함수 노드, depth-1 엣지. 모듈로 묶인 전체 지도.
- `drilldown-main`: main 의 depth-1 직접 호출만(디스패처라 전이 전개 시 전체 프로그램이 되므로).
- `drilldown-<유스케이스>` 16장: 각 `execute` 의 의미 노드만(보통 2~12). ctor/dtor/getter/익명ns 헬퍼 제외됨.
