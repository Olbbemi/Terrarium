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

# --- 코어 라이브러리 (seed/stem/stomata → libplanning_core) ---
add_library(planning_core
    src/seed/TimeRange.cpp
    src/seed/Event.cpp
    src/seed/Todo.cpp
    src/seed/Goal.cpp
    src/seed/ConflictDetector.cpp
    src/seed/StdUuidGenerator.cpp
    src/stem/CreateEventUseCase.cpp
    src/stem/UpdateEventUseCase.cpp
    src/stem/DeleteEventUseCase.cpp
    src/stem/ListEventsUseCase.cpp
    src/stem/AddTodoUseCase.cpp
    src/stem/MarkTodoDoneUseCase.cpp
    src/stem/DeleteTodoUseCase.cpp
    src/stem/UpdateTodoUseCase.cpp
    src/stem/ListTodosUseCase.cpp
    src/stem/CreateGoalUseCase.cpp
    src/stem/UpdateGoalUseCase.cpp
    src/stem/DeleteGoalUseCase.cpp
    src/stem/LogGoalUseCase.cpp
    src/stem/ShowGoalUseCase.cpp
    src/stem/ListGoalsUseCase.cpp
    src/stem/ShowDashboardUseCase.cpp
)
target_include_directories(planning_core PUBLIC src nutrients/stduuid/include)

# --- 어댑터: rings (로깅, spdlog) ---
add_library(planning_rings
    src/rings/SpdlogLogger.cpp
)
target_include_directories(planning_rings PUBLIC src)
target_link_libraries(planning_rings PUBLIC planning_core PRIVATE spdlog::spdlog)

# --- 어댑터: climate (설정, toml++) ---
add_library(planning_climate
    src/climate/TomlConfigLoader.cpp
    src/climate/DefaultConfig.cpp
)
target_include_directories(planning_climate PUBLIC src PRIVATE nutrients/tomlplusplus/include)
target_link_libraries(planning_climate PUBLIC planning_core)

# --- 어댑터: roots (SQLite 저장, SQLiteCpp) ---
add_library(planning_roots
    src/roots/MigrationRunner.cpp
    src/roots/SqliteEventRepository.cpp
    src/roots/SqliteTodoRepository.cpp
    src/roots/SqliteGoalRepository.cpp
)
target_include_directories(planning_roots PUBLIC src)
target_link_libraries(planning_roots PUBLIC planning_core PRIVATE SQLiteCpp)

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
    planning_core planning_roots planning_rings planning_climate planning_leaves SQLiteCpp)

# --- 테스트 ---
add_executable(terrarium_tests
    observation/smoke_test.cpp
    observation/seed/time_range_test.cpp
    observation/seed/event_test.cpp
    observation/seed/todo_test.cpp
    observation/seed/goal_test.cpp
    observation/seed/conflict_detector_test.cpp
    observation/stem/create_event_usecase_test.cpp
    observation/stem/update_event_usecase_test.cpp
    observation/stem/delete_event_usecase_test.cpp
    observation/stem/list_events_usecase_test.cpp
    observation/stem/add_todo_usecase_test.cpp
    observation/stem/mark_todo_done_usecase_test.cpp
    observation/stem/delete_todo_usecase_test.cpp
    observation/stem/update_todo_usecase_test.cpp
    observation/stem/list_todos_usecase_test.cpp
    observation/stem/create_goal_usecase_test.cpp
    observation/stem/update_goal_usecase_test.cpp
    observation/stem/delete_goal_usecase_test.cpp
    observation/stem/log_goal_usecase_test.cpp
    observation/stem/show_goal_usecase_test.cpp
    observation/stem/list_goals_usecase_test.cpp
    observation/stem/show_dashboard_usecase_test.cpp
    observation/rings/spdlog_logger_test.cpp
    observation/climate/toml_config_loader_test.cpp
    observation/climate/render_default_config_test.cpp
    observation/roots/migration_runner_test.cpp
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
    planning_core planning_rings planning_climate planning_roots planning_leaves SQLiteCpp GTest::gtest_main)
gtest_discover_tests(terrarium_tests)
