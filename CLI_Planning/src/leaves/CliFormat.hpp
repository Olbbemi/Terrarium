#pragma once

#include <chrono>
#include <string>

#include "seed/Priority.hpp"

// CLI 입출력 변환(엣지 어댑터). 문자열 ↔ 도메인 타입, 로컬↔UTC 타임존 변환(정책 A),
// 진행 막대 렌더링을 모은다. main(glass)에 두면 테스트 바이너리가 링크할 수 없어
// 별도 유닛으로 분리한다.
namespace planning::adapter_cli {

// 로컬 civil(broken-down) 시각 → UTC sys_seconds (시스템 로컬 타임존 기준).
std::chrono::sys_seconds localCivilToUtc(int y, int mo, int d, int h, int mi);

// "YYYY-MM-DDTHH:MM" 를 로컬 시각으로 해석해 UTC sys_seconds 로. 형식 오류 시 std::runtime_error.
std::chrono::sys_seconds parseDateTime(const std::string& s);

// "YYYY-MM-DD" → 달력 날짜(sys_days). 형식 오류 시 std::runtime_error.
std::chrono::sys_days parseDate(const std::string& s);

// "high|medium|low" → Priority. 그 외 값이면 std::runtime_error.
domain::Priority parsePriority(const std::string& s);

// Priority → "high|medium|low".
const char* priorityText(domain::Priority p);

// sys_days → "YYYY-MM-DD".
std::string formatDate(std::chrono::sys_days d);

// UTC sys_seconds → 로컬 "YYYY-MM-DD HH:MM" 표시(정책 A).
std::string formatDateTime(std::chrono::sys_seconds t);

// 로컬 달력 날짜의 자정 → UTC instant (event 범위 경계용).
std::chrono::sys_seconds localMidnightUtc(std::chrono::sys_days date);

// 달성률(0.0~) → ASCII 막대. 초과 달성은 막대 가득참으로 클램프.
std::string progressBar(double ratio, int width = 10);

}  // namespace planning::adapter_cli
