cmake_minimum_required(VERSION 3.20)
project(Terrarium LANGUAGES CXX)

# --- C++20 표준 (설계 NF5: GCC 13+ / Clang 16+) ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- compile_commands.json 자동 생성 (clangd / code-analyze 호출그래프용) ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- 컴파일러 경고 (cpp.md 권장) ---
add_compile_options(-Wall -Wextra -Wpedantic)

# --- 테스트 (CTest 루트는 최상위) ---
enable_testing()

# --- 앱 우산: orchard/ (현재 앱 1개: CLI_Planning) ---
add_subdirectory(orchard/CLI_Planning)

# --- 공유 범용 인프라: toolshed/ (A3 log 채움 / A4 sqlite) ---
# orchard 뒤에 둔다: toolshed/log 가 orchard 하위 nutrients 의 spdlog::spdlog 타깃에 링크.
add_subdirectory(toolshed)
