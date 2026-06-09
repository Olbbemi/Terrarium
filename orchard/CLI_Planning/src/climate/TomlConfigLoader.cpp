#include "climate/TomlConfigLoader.hpp"

#include <stdexcept>
#include <string>

#include <toml++/toml.hpp>

namespace planning::adapter_config {

TomlConfigLoader::TomlConfigLoader(const std::filesystem::path& configFile) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(configFile.string());
    } catch (const toml::parse_error& e) {
        // 파일 없음 / 파싱 오류 모두 여기로 (toml++ 가 둘 다 parse_error 로 던짐).
        throw std::runtime_error(std::string("config parse failed: ") +
                                 e.description().data());
    }

    // 필수 필드.
    auto db = tbl["database"]["path"].value<std::string>();
    if (!db) throw std::runtime_error("config missing required: database.path");
    dbPath_ = *db;

    auto logPath = tbl["log"]["path"].value<std::string>();
    if (!logPath) throw std::runtime_error("config missing required: log.path");
    log_.path = *logPath;

    // 선택 필드(기본값).
    log_.level = tbl["log"]["level"].value_or<std::string>("INFO");
    log_.audit = tbl["log"]["audit"].value_or(true);
    log_.rotationStrategy = tbl["log"]["rotation"].value_or<std::string>("none");
    log_.debugRetentionDays = tbl["log"]["debug_retention_days"].value_or(7);
    log_.auditRetentionDays = tbl["log"]["audit_retention_days"].value_or(365);
    log_.separateDebugAudit = tbl["log"]["separate_debug_audit"].value_or(false);
}

ports::ConfigLoader::LogConfig TomlConfigLoader::logConfig() const {
    return log_;
}

std::filesystem::path TomlConfigLoader::dbPath() const { return dbPath_; }

}  // namespace planning::adapter_config
