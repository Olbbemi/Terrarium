#pragma once

#include <memory>
#include <string>

#include "stomata/ConfigLoader.hpp"
#include "stomata/Logger.hpp"

namespace spdlog {
class logger;
}

namespace planning::adapter_logger {

// spdlog 기반 Logger 구현. 디버그/감사 싱크 분리, 파일 실패 시 stderr 폴백.
class SpdlogLogger : public ports::Logger {
public:
    explicit SpdlogLogger(const ports::ConfigLoader::LogConfig& config);
    ~SpdlogLogger() override;

    void debug(const std::string&) override;
    void info(const std::string&) override;
    void warn(const std::string&) override;
    void error(const std::string&) override;
    void audit(const std::string& action, const std::string& detail) override;

private:
    std::shared_ptr<spdlog::logger> debug_;
    std::shared_ptr<spdlog::logger> audit_;
    bool auditEnabled_;
};

}  // namespace planning::adapter_logger
