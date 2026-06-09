#include "climate/DefaultConfig.hpp"

#include <sstream>

#include <toml++/toml.hpp>

namespace planning::config {

std::string renderDefaultConfig(const std::filesystem::path& dbPath,
                                const std::filesystem::path& logPath) {
    // toml++ 는 직렬화 시 키를 정렬하므로 출력 순서는 보장되지 않는다(로드에는 무관).
    toml::table database;
    database.insert("path", dbPath.string());

    toml::table log;
    log.insert("path", logPath.string());
    log.insert("level", "INFO");
    log.insert("audit", true);
    log.insert("rotation", "none");
    log.insert("debug_retention_days", 7);
    log.insert("audit_retention_days", 365);
    log.insert("separate_debug_audit", false);

    toml::table root;
    root.insert("database", std::move(database));
    root.insert("log", std::move(log));

    std::ostringstream ss;
    ss << root << '\n';
    return ss.str();
}

}  // namespace planning::config
