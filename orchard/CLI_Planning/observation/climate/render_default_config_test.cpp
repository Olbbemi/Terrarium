#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "climate/DefaultConfig.hpp"
#include "climate/TomlConfigLoader.hpp"

using planning::config::renderDefaultConfig;
using planning::config::TomlConfigLoader;

namespace {

namespace fs = std::filesystem;

fs::path writeTo(const char* name, const std::string& content) {
    fs::path dir = fs::path(::testing::TempDir()) / "climate_render";
    fs::create_directories(dir);
    fs::path file = dir / name;
    std::ofstream(file) << content;
    return file;
}

}  // namespace

// 핵심 계약: init 이 생성한 설정은 TomlConfigLoader 로 그대로 로드되어야 하고,
// 주어진 db/log 경로를 담고, 선택 필드는 기본값으로 채워져야 한다.
TEST(RenderDefaultConfig, generated_config_loads_with_given_paths_and_defaults) {
    const std::string toml =
        renderDefaultConfig("/data/terra.db", "/data/terra.log");
    auto file = writeTo("generated.toml", toml);

    TomlConfigLoader loader(file);
    EXPECT_EQ(loader.dbPath(), fs::path("/data/terra.db"));
    auto lc = loader.logConfig();
    EXPECT_EQ(lc.path, fs::path("/data/terra.log"));
    EXPECT_EQ(lc.level, "INFO");
    EXPECT_TRUE(lc.audit);
    EXPECT_EQ(lc.rotationStrategy, "none");
    EXPECT_EQ(lc.debugRetentionDays, 7);
    EXPECT_EQ(lc.auditRetentionDays, 365);
    EXPECT_FALSE(lc.separateDebugAudit);
}

// 경로에 공백/특수문자가 있어도 TOML 직렬화가 깨지지 않아야 한다.
TEST(RenderDefaultConfig, escapes_paths_with_spaces) {
    const std::string toml =
        renderDefaultConfig("/data dir/terra.db", "/data dir/terra.log");
    auto file = writeTo("spaced.toml", toml);

    TomlConfigLoader loader(file);
    EXPECT_EQ(loader.dbPath(), fs::path("/data dir/terra.db"));
    EXPECT_EQ(loader.logConfig().path, fs::path("/data dir/terra.log"));
}
