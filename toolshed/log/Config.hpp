#pragma once

#include <filesystem>
#include <string>

namespace toolshed::log {

// 로깅 주체별 범용 설정 (앱 무관). 앱(planning::config)이 자기 스키마에서 매핑한다.
struct Config {
    std::string name;              // 주체별 고유 식별자 (spdlog 레지스트리 충돌 방지; 8-B-4에서 활성)
    std::filesystem::path path;
    std::string level;             // "DEBUG"/"INFO"/"WARN"/"ERROR"
    bool audit;
    std::string rotation;          // "daily"/"size"/"none"
    int debugRetentionDays;
    int auditRetentionDays;
    bool separateDebugAudit;
};

}  // namespace toolshed::log
