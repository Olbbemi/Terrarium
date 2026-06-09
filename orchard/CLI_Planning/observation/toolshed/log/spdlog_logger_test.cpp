#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "toolshed/log/Config.hpp"
#include "toolshed/log/SpdlogLogger.hpp"

using toolshed::log::SpdlogLogger;
using LogConfig = toolshed::log::Config;

namespace {

namespace fs = std::filesystem;

fs::path uniqueDir(const char* name) {
    fs::path d = fs::path(::testing::TempDir()) / (std::string("rings_") + name);
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

std::string readAll(const fs::path& p) {
    std::ifstream in(p);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

LogConfig baseConfig(const fs::path& logPath) {
    LogConfig c;
    c.name = "test";
    c.path = logPath;
    c.level = "DEBUG";
    c.audit = true;
    c.rotation = "none";
    c.debugRetentionDays = 0;
    c.auditRetentionDays = 0;
    c.separateDebugAudit = true;
    return c;
}

}  // namespace

TEST(SpdlogLogger, debug_writes_to_debug_sink) {
    auto dir = uniqueDir("debug");
    auto logPath = dir / "app.log";
    {
        SpdlogLogger logger(baseConfig(logPath));
        logger.debug("hello-debug");
    }
    EXPECT_NE(readAll(logPath).find("hello-debug"), std::string::npos);
}

TEST(SpdlogLogger, audit_writes_to_audit_sink_when_separate) {
    auto dir = uniqueDir("audit");
    auto logPath = dir / "app.log";
    auto auditPath = dir / "app.audit.log";
    {
        SpdlogLogger logger(baseConfig(logPath));
        logger.audit("event.create", "created");
    }
    const std::string auditContent = readAll(auditPath);
    EXPECT_NE(auditContent.find("event.create"), std::string::npos);
    // 분리 모드: 디버그 파일에는 감사 항목이 없어야 한다.
    EXPECT_EQ(readAll(logPath).find("event.create"), std::string::npos);
}

TEST(SpdlogLogger, daily_rotation_creates_date_stamped_file) {
    auto dir = uniqueDir("daily");
    LogConfig cfg = baseConfig(dir / "app.log");
    cfg.rotation = "daily";
    cfg.separateDebugAudit = false;
    {
        SpdlogLogger logger(cfg);
        logger.debug("rotate-me");
    }
    // daily_file_sink 는 "app_<YYYY-MM-DD>.log" 형태로 생성한다.
    bool found = false;
    for (const auto& e : fs::directory_iterator(dir)) {
        const auto name = e.path().filename().string();
        if (name.rfind("app_", 0) == 0 && e.path().extension() == ".log") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(SpdlogLogger, filesystem_error_falls_back_to_stderr) {
    auto dir = uniqueDir("fserr");
    // 일반 파일을 부모 경로로 둬서 디렉토리 생성/열기를 실패하게 만든다.
    auto blocker = dir / "blocker";
    std::ofstream(blocker) << "x";
    LogConfig cfg = baseConfig(blocker / "app.log");  // blocker 는 파일 → 경로 불가

    // 생성자가 예외를 밖으로 던지지 않고 stderr 로 폴백해야 한다.
    EXPECT_NO_THROW({
        SpdlogLogger logger(cfg);
        logger.info("fallback works");
    });
}
