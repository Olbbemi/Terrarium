# toolshed/log — 범용 Logger facade (Logger 인터페이스 + spdlog 구현).
# 도메인-free: orchard(planning::) 를 절대 import 하지 않는다. OSS spdlog 만 의존.
add_library(toolshed_log
    SpdlogLogger.cpp
)
# 소비자가 "toolshed/log/Logger.hpp" 로 include 하도록 Terrarium 루트를 노출.
target_include_directories(toolshed_log PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(toolshed_log PRIVATE spdlog::spdlog)
