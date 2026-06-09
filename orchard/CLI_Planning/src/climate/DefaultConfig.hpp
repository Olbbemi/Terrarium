#pragma once

#include <filesystem>
#include <string>

namespace planning::config {

// 주어진 db/log 경로로 기본 설정 TOML 텍스트를 생성한다(init 명령용).
// 선택 필드(level/audit/rotation/보존일수)는 TomlConfigLoader 기본값과 동일하게 채운다.
// 경로 이스케이프는 toml++ 직렬화에 위임한다.
std::string renderDefaultConfig(const std::filesystem::path& dbPath,
                                const std::filesystem::path& logPath);

}  // namespace planning::config
