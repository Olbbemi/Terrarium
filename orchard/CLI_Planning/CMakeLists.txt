# orchard/CLI_Planning — 앱 서브프로젝트.
# 전역 설정(cmake_minimum/project/C++표준/경고/compile_commands/enable_testing)은
# 최상위 Terrarium/CMakeLists.txt 로 이동. 여기서는 nutrients + 타깃만 둔다.

# --- 외부 의존성: nutrients/ (in-tree vendoring, NF6 빌드 시 다운로드 0) ---
add_subdirectory(nutrients/googletest)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(nutrients/spdlog)

set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "" FORCE)  # 시스템 sqlite3 링크 (NF6)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
add_subdirectory(nutrients/SQLiteCpp)

# --- 테스트 (enable_testing 은 최상위 Terrarium/CMakeLists.txt) ---
include(GoogleTest)

# --- 코어 라이브러리 (trunk: domain/usecase/ports → libplanning_core) ---
add_library(planning_core
    src/trunk/domain/TimeRange.cpp
    src/trunk/domain/Event.cpp
    src/trunk/domain/Todo.cpp
    src/trunk/domain/Goal.cpp
    src/trunk/domain/ConflictDetector.cpp
    src/trunk/domain/StdUuidGenerator.cpp
    src/trunk/usecase/CreateEventUseCase.cpp
    src/trunk/usecase/UpdateEventUseCase.cpp
    src/trunk/usecase/DeleteEventUseCase.cpp
    src/trunk/usecase/ListEventsUseCase.cpp
    src/trunk/usecase/AddTodoUseCase.cpp
    src/trunk/usecase/MarkTodoDoneUseCase.cpp
    src/trunk/usecase/DeleteTodoUseCase.cpp
    src/trunk/usecase/UpdateTodoUseCase.cpp
    src/trunk/usecase/ListTodosUseCase.cpp
    src/trunk/usecase/CreateGoalUseCase.cpp
    src/trunk/usecase/UpdateGoalUseCase.cpp
    src/trunk/usecase/DeleteGoalUseCase.cpp
    src/trunk/usecase/LogGoalUseCase.cpp
    src/trunk/usecase/ShowGoalUseCase.cpp
    src/trunk/usecase/ListGoalsUseCase.cpp
    src/trunk/usecase/ShowDashboardUseCase.cpp
)
target_include_directories(planning_core PUBLIC src nutrients/stduuid/include)
# usecase 가 toolshed::log::Logger 에 의존(공유 facade). toolshed 는 Terrarium/toolshed 에 정의.
target_link_libraries(planning_core PUBLIC toolshed_log)

# --- 어댑터: climate (설정, toml++) ---
add_library(planning_climate
    src/climate/TomlConfigLoader.cpp
    src/climate/DefaultConfig.cpp
)
target_include_directories(planning_climate PUBLIC src PRIVATE nutrients/tomlplusplus/include)
target_link_libraries(planning_climate PUBLIC planning_core)

# --- 어댑터: roots (도메인-인지 SQLite 저장소; 연결/마이그레이션은 toolshed_sqlite) ---
add_library(planning_roots
    src/roots/SqliteEventRepository.cpp
    src/roots/SqliteTodoRepository.cpp
    src/roots/SqliteGoalRepository.cpp
)
target_include_directories(planning_roots PUBLIC src)
target_link_libraries(planning_roots PUBLIC planning_core PRIVATE SQLiteCpp toolshed_sqlite)

# --- 어댑터: leaves (CLI driving) ---
add_library(planning_leaves
    src/leaves/CliConflictPrompter.cpp
    src/leaves/CliFormat.cpp
)
target_include_directories(planning_leaves PUBLIC src)
target_link_libraries(planning_leaves PUBLIC planning_core)

# --- 실행 파일: gaia (Composition Root) ---
add_executable(gaia src/glass/main.cpp)
target_include_directories(gaia PRIVATE src nutrients/CLI11/include)
target_compile_definitions(gaia PRIVATE
    TERRARIUM_MIGRATIONS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/seasons")
target_link_libraries(gaia PRIVATE
    planning_core planning_roots toolshed_sqlite toolshed_log planning_climate planning_leaves SQLiteCpp)

# --- 테스트 ---
add_executable(terrarium_tests
    observation/smoke_test.cpp
    observation/trunk/domain/time_range_test.cpp
    observation/trunk/domain/event_test.cpp
    observation/trunk/domain/todo_test.cpp
    observation/trunk/domain/goal_test.cpp
    observation/trunk/domain/conflict_detector_test.cpp
    observation/trunk/usecase/create_event_usecase_test.cpp
    observation/trunk/usecase/update_event_usecase_test.cpp
    observation/trunk/usecase/delete_event_usecase_test.cpp
    observation/trunk/usecase/list_events_usecase_test.cpp
    observation/trunk/usecase/add_todo_usecase_test.cpp
    observation/trunk/usecase/mark_todo_done_usecase_test.cpp
    observation/trunk/usecase/delete_todo_usecase_test.cpp
    observation/trunk/usecase/update_todo_usecase_test.cpp
    observation/trunk/usecase/list_todos_usecase_test.cpp
    observation/trunk/usecase/create_goal_usecase_test.cpp
    observation/trunk/usecase/update_goal_usecase_test.cpp
    observation/trunk/usecase/delete_goal_usecase_test.cpp
    observation/trunk/usecase/log_goal_usecase_test.cpp
    observation/trunk/usecase/show_goal_usecase_test.cpp
    observation/trunk/usecase/list_goals_usecase_test.cpp
    observation/trunk/usecase/show_dashboard_usecase_test.cpp
    observation/toolshed/log/spdlog_logger_test.cpp
    observation/toolshed/sqlite/database_test.cpp
    observation/toolshed/sqlite/migration_runner_test.cpp
    observation/climate/toml_config_loader_test.cpp
    observation/climate/render_default_config_test.cpp
    observation/roots/sqlite_event_repository_test.cpp
    observation/roots/sqlite_todo_repository_test.cpp
    observation/roots/sqlite_goal_repository_test.cpp
    observation/leaves/cli_conflict_prompter_test.cpp
    observation/leaves/cli_format_test.cpp
)
target_include_directories(terrarium_tests PRIVATE observation)
target_compile_definitions(terrarium_tests PRIVATE
    TERRARIUM_MIGRATIONS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/seasons")
target_link_libraries(terrarium_tests PRIVATE
    planning_core toolshed_sqlite toolshed_log planning_climate planning_roots planning_leaves SQLiteCpp GTest::gtest_main)
gtest_discover_tests(terrarium_tests)
