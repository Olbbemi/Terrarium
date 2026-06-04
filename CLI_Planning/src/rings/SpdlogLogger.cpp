#include "rings/SpdlogLogger.hpp"

#include <cstdint>
#include <filesystem>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

namespace planning::adapter_logger {

namespace {

spdlog::level::level_enum toLevel(const std::string& s) {
    if (s == "DEBUG") return spdlog::level::debug;
    if (s == "INFO") return spdlog::level::info;
    if (s == "WARN") return spdlog::level::warn;
    if (s == "ERROR") return spdlog::level::err;
    return spdlog::level::info;
}

spdlog::sink_ptr makeSink(const std::filesystem::path& path,
                          const std::string& rotation, int retentionDays) {
    if (rotation == "daily") {
        return std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            path.string(), 0, 0, false,
            static_cast<uint16_t>(retentionDays > 0 ? retentionDays : 0));
    }
    if (rotation == "size") {
        return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            path.string(), 5 * 1024 * 1024, 3);
    }
    return std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(),
                                                               false);
}

// 분리 모드의 감사 로그 경로: "<stem>.audit<ext>".
std::filesystem::path auditPath(const std::filesystem::path& p) {
    return p.parent_path() / (p.stem().string() + ".audit" + p.extension().string());
}

}  // namespace

SpdlogLogger::SpdlogLogger(const ports::ConfigLoader::LogConfig& config)
    : auditEnabled_(config.audit) {
    try {
        auto debugSink =
            makeSink(config.path, config.rotationStrategy, config.debugRetentionDays);
        debug_ = std::make_shared<spdlog::logger>("debug", debugSink);
        debug_->set_level(toLevel(config.level));
        debug_->flush_on(spdlog::level::trace);

        spdlog::sink_ptr auditSink =
            config.separateDebugAudit
                ? makeSink(auditPath(config.path), config.rotationStrategy,
                           config.auditRetentionDays)
                : debugSink;
        audit_ = std::make_shared<spdlog::logger>("audit", auditSink);
        audit_->set_level(spdlog::level::info);
        audit_->flush_on(spdlog::level::trace);
    } catch (const spdlog::spdlog_ex&) {
        // 파일 싱크 생성 실패 → stderr 폴백(예외를 밖으로 던지지 않음).
        auto err = std::make_shared<spdlog::sinks::stderr_sink_mt>();
        debug_ = std::make_shared<spdlog::logger>("debug", err);
        audit_ = std::make_shared<spdlog::logger>("audit", err);
        debug_->flush_on(spdlog::level::trace);
        audit_->flush_on(spdlog::level::trace);
    }
}

SpdlogLogger::~SpdlogLogger() = default;

void SpdlogLogger::debug(const std::string& m) { debug_->debug(m); }
void SpdlogLogger::info(const std::string& m) { debug_->info(m); }
void SpdlogLogger::warn(const std::string& m) { debug_->warn(m); }
void SpdlogLogger::error(const std::string& m) { debug_->error(m); }

void SpdlogLogger::audit(const std::string& action, const std::string& detail) {
    if (!auditEnabled_) return;
    audit_->info("[AUDIT] " + action + " | " + detail);
}

}  // namespace planning::adapter_logger
