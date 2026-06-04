#pragma once

#include <filesystem>
#include <string>

namespace planning::ports {

class ConfigLoader {
public:
    struct LogConfig {
        std::filesystem::path path;
        std::string level;             // "DEBUG"/"INFO"/"WARN"/"ERROR"
        bool audit;
        std::string rotationStrategy;  // "daily"/"size"/"none"
        int debugRetentionDays;
        int auditRetentionDays;
        bool separateDebugAudit;
    };

    virtual ~ConfigLoader() = default;
    virtual LogConfig logConfig() const = 0;
    virtual std::filesystem::path dbPath() const = 0;
};

}  // namespace planning::ports
