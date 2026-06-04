cmake_minimum_required(VERSION 3.20)
project(Terrarium LANGUAGES CXX)

# --- C++20 표준 (설계 NF5: GCC 13+ / Clang 16+) ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 컴파일러 경고 (cpp.md 권장) ---
add_compile_options(-Wall -Wextra -Wpedantic)

# --- 외부 의존성: nutrients/ (in-tree vendoring, NF6 빌드 시 다운로드 0) ---
add_subdirectory(nutrients/googletest)

# --- 테스트 ---
enable_testing()
include(GoogleTest)

# --- 코어 라이브러리 (seed/stem/stomata → libplanning_core) ---
add_library(planning_core
    src/seed/TimeRange.cpp
    src/seed/Event.cpp
    src/seed/Todo.cpp
    src/seed/Goal.cpp
    src/seed/ConflictDetector.cpp
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
)
target_include_directories(terrarium_tests PRIVATE observation)
target_link_libraries(terrarium_tests PRIVATE planning_core GTest::gtest_main)
gtest_discover_tests(terrarium_tests)
