#include "leaves/CliFormat.hpp"

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace planning::adapter_cli {

// mktime 이 시스템 로컬 타임존 기준으로 해석해 epoch 초로 변환한다.
// (GCC 11 libstdc++ 는 std::chrono::current_zone 미지원 → POSIX <ctime> 사용.)
std::chrono::sys_seconds localCivilToUtc(int y, int mo, int d, int h, int mi) {
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;  // DST 여부 자동 판단
    const std::time_t t = std::mktime(&tm);
    return std::chrono::sys_seconds{
        std::chrono::seconds{static_cast<std::int64_t>(t)}};
}

std::chrono::sys_seconds parseDateTime(const std::string& s) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) != 5) {
        throw std::runtime_error("잘못된 날짜시간 형식 (YYYY-MM-DDTHH:MM): " + s);
    }
    return localCivilToUtc(y, mo, d, h, mi);
}

std::chrono::sys_days parseDate(const std::string& s) {
    int y = 0, mo = 0, d = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &d) != 3) {
        throw std::runtime_error("잘못된 날짜 형식 (YYYY-MM-DD): " + s);
    }
    return std::chrono::sys_days{std::chrono::year{y} /
                                 std::chrono::month{static_cast<unsigned>(mo)} /
                                 std::chrono::day{static_cast<unsigned>(d)}};
}

domain::Priority parsePriority(const std::string& s) {
    if (s == "high") return domain::Priority::HIGH;
    if (s == "medium") return domain::Priority::MEDIUM;
    if (s == "low") return domain::Priority::LOW;
    throw std::runtime_error("우선순위는 high|medium|low 중 하나여야 합니다: " + s);
}

const char* priorityText(domain::Priority p) {
    switch (p) {
        case domain::Priority::HIGH: return "high";
        case domain::Priority::MEDIUM: return "medium";
        case domain::Priority::LOW: return "low";
    }
    return "?";
}

std::string formatDate(std::chrono::sys_days d) {
    const std::chrono::year_month_day ymd{d};
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()),
                  static_cast<unsigned>(ymd.day()));
    return buf;
}

std::string formatDateTime(std::chrono::sys_seconds t) {
    const std::time_t tt = static_cast<std::time_t>(t.time_since_epoch().count());
    std::tm lt{};
    ::localtime_r(&tt, &lt);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d", lt.tm_year + 1900,
                  lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min);
    return buf;
}

std::chrono::sys_seconds localMidnightUtc(std::chrono::sys_days date) {
    const std::chrono::year_month_day ymd{date};
    return localCivilToUtc(static_cast<int>(ymd.year()),
                           static_cast<int>(static_cast<unsigned>(ymd.month())),
                           static_cast<int>(static_cast<unsigned>(ymd.day())), 0, 0);
}

std::string progressBar(double ratio, int width) {
    int filled = static_cast<int>(ratio * width + 0.5);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    std::string bar(static_cast<std::size_t>(filled), '#');
    bar.append(static_cast<std::size_t>(width - filled), '-');
    return "[" + bar + "]";
}

}  // namespace planning::adapter_cli
