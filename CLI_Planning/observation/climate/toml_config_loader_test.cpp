#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "climate/TomlConfigLoader.hpp"

using planning::adapter_config::TomlConfigLoader;

namespace {

namespace fs = std::filesystem;

fs::path writeConfig(const char* name, const std::string& content) {
    fs::path dir = fs::path(::testing::TempDir()) / "climate";
    fs::create_directories(dir);
    fs::path file = dir / name;
    std::ofstream(file) << content;
    return file;
}

const char* kFull = R"(
[database]
path = "/var/data/gaia.db"

[log]
path = "/var/log/gaia.log"
level = "WARN"
audit = false
rotation = "daily"
debug_retention_days = 14
audit_retention_days = 400
separate_debug_audit = true
)";

}  // namespace

TEST(TomlConfigLoader, parses_valid_config_and_matches_values) {
    auto file = writeConfig("full.toml", kFull);
    TomlConfigLoader loader(file);

    EXPECT_EQ(loader.dbPath(), fs::path("/var/data/gaia.db"));
    auto lc = loader.logConfig();
    EXPECT_EQ(lc.path, fs::path("/var/log/gaia.log"));
    EXPECT_EQ(lc.level, "WARN");
    EXPECT_FALSE(lc.audit);
    EXPECT_EQ(lc.rotationStrategy, "daily");
    EXPECT_EQ(lc.debugRetentionDays, 14);
    EXPECT_EQ(lc.auditRetentionDays, 400);
    EXPECT_TRUE(lc.separateDebugAudit);
}

TEST(TomlConfigLoader, missing_file_throws) {
    EXPECT_THROW(TomlConfigLoader(fs::path("/no/such/config_xyz.toml")),
                 std::runtime_error);
}

TEST(TomlConfigLoader, invalid_toml_throws) {
    auto file = writeConfig("bad.toml", "this is = = not valid toml [[[");
    EXPECT_THROW({ TomlConfigLoader loader(file); }, std::runtime_error);
}

TEST(TomlConfigLoader, missing_required_field_throws) {
    // database.path 누락
    auto file = writeConfig("noreq.toml", "[log]\npath = \"/var/log/x.log\"\n");
    EXPECT_THROW({ TomlConfigLoader loader(file); }, std::runtime_error);
}

TEST(TomlConfigLoader, optional_field_uses_default) {
    // 필수만 두고 level 등 생략 → 기본값.
    auto file = writeConfig(
        "minimal.toml",
        "[database]\npath = \"/d.db\"\n[log]\npath = \"/l.log\"\n");
    TomlConfigLoader loader(file);
    auto lc = loader.logConfig();
    EXPECT_EQ(lc.level, "INFO");           // 기본값
    EXPECT_TRUE(lc.audit);                  // 기본값
    EXPECT_EQ(lc.rotationStrategy, "none");  // 기본값
    EXPECT_FALSE(lc.separateDebugAudit);    // 기본값
}
