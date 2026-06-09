#pragma once

#include <filesystem>

#include "trunk/ports/ConfigLoader.hpp"

namespace planning::config {

// TOML 설정 파일을 읽어 ConfigLoader 를 구현. 파일/파싱/필수필드 오류 시 std::runtime_error.
class TomlConfigLoader : public ports::ConfigLoader {
public:
    explicit TomlConfigLoader(const std::filesystem::path& configFile);

    LogConfig logConfig() const override;
    std::filesystem::path dbPath() const override;

private:
    LogConfig log_;
    std::filesystem::path dbPath_;
};

}  // namespace planning::config
