#pragma once

#include <chrono>
#include <string>

#include "trunk/domain/Priority.hpp"
#include "trunk/domain/RecurrenceRule.hpp"

// CLI 입출력 변환(엣지 어댑터). 문자열 ↔ 도메인 타입, 로컬↔UTC 타임존 변환(정책 A),
// 진행 막대 렌더링을 모은다. main(glass)에 두면 테스트 바이너리가 링크할 수 없어
// 별도 유닛으로 분리한다.
//
// 타임존 변환은 std::chrono 타임존 라이브러리를 쓴다. zone 인자는 기본값으로
// current_zone()(시스템 로컬, 정책 A)을 쓰되, 테스트는 locate_zone 으로 명시 주입한다.
// (libstdc++ current_zone() 은 TZ 환경변수를 무시하고 /etc/localtime 만 보므로 주입형으로 설계.)
namespace planning::ui {

// 로컬 civil(broken-down) 시각 → UTC sys_seconds.
std::chrono::sys_seconds localCivilToUtc(
    int y, int mo, int d, int h, int mi,
    const std::chrono::time_zone* zone = std::chrono::current_zone());

// "YYYY-MM-DDTHH:MM" 를 로컬 시각으로 해석해 UTC sys_seconds 로. 형식 오류 시 std::runtime_error.
std::chrono::sys_seconds parseDateTime(
    const std::string& s,
    const std::chrono::time_zone* zone = std::chrono::current_zone());

// "YYYY-MM-DD" → 달력 날짜(sys_days). 형식 오류 시 std::runtime_error.
std::chrono::sys_days parseDate(const std::string& s);

// "high|medium|low" → Priority. 그 외 값이면 std::runtime_error.
domain::Priority parsePriority(const std::string& s);

// Priority → "high|medium|low".
const char* priorityText(domain::Priority p);

// sys_days → "YYYY-MM-DD".
std::string formatDate(std::chrono::sys_days d);

// UTC sys_seconds → 로컬 "YYYY-MM-DD HH:MM" 표시(정책 A).
std::string formatDateTime(
    std::chrono::sys_seconds t,
    const std::chrono::time_zone* zone = std::chrono::current_zone());

// 로컬 달력 날짜의 자정 → UTC instant (event 범위 경계용).
std::chrono::sys_seconds localMidnightUtc(
    std::chrono::sys_days date,
    const std::chrono::time_zone* zone = std::chrono::current_zone());

// 시스템 로컬 타임존 기준 오늘 달력 날짜(정책 A). 벽시계(now)를 읽는다.
std::chrono::sys_days localToday(
    const std::chrono::time_zone* zone = std::chrono::current_zone());

// 달성률(0.0~) → ASCII 막대. 초과 달성은 막대 가득참으로 클램프.
std::string progressBar(double ratio, int width = 10);

// "daily|weekly|monthly|yearly" → RecurrenceFrequency. 그 외 값이면 std::runtime_error.
domain::RecurrenceFrequency parseFrequency(const std::string& s);

// RecurrenceFrequency → "매일|매주|매월|매년".
const char* frequencyText(domain::RecurrenceFrequency f);

}  // namespace planning::ui
