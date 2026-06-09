# toolshed/sqlite — 범용 sqlite 플럼빙 (연결 RAII + 마이그레이션 메커니즘).
# 도메인-free: orchard(planning::) 를 절대 import 하지 않는다. OSS SQLiteCpp 만 의존.
# 공개 헤더(Database.hpp)가 SQLite::Database 를 노출하므로 SQLiteCpp 는 PUBLIC.
add_library(toolshed_sqlite
    Database.cpp
    MigrationRunner.cpp
)
# 소비자가 "toolshed/sqlite/Database.hpp" 로 include 하도록 Terrarium 루트를 노출.
target_include_directories(toolshed_sqlite PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(toolshed_sqlite PUBLIC SQLiteCpp)
