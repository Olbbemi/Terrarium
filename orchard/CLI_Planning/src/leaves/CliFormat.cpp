#include "leaves/CliFormat.hpp"

#include <cstdio>
#include <stdexcept>

namespace planning::ui {

using namespace std::chrono;

// 로컬 civil 시각을 주어진 타임존으로 해석해 UTC instant 로 변환.
sys_seconds localCivilToUtc(int y, int mo, int d, int h, int mi,
                            const time_zone* zone) {
    const local_seconds lt =
        local_days{year{y} / month{static_cast<unsigned>(mo)} /
                   day{static_cast<unsigned>(d)}} +
        hours{h} + minutes{mi};
    return zone->to_sys(lt);  // 모호/존재안함 시각이면 표준 예외를 던진다(KST 는 무관)
}

sys_seconds parseDateTime(const std::string& s, const time_zone* zone) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) != 5) {
        throw std::runtime_error("잘못된 날짜시간 형식 (YYYY-MM-DDTHH:MM): " + s);
    }
    return localCivilToUtc(y, mo, d, h, mi, zone);
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

std::string formatDateTime(std::chrono::sys_seconds t, const time_zone* zone) {
    const local_seconds lt = zone->to_local(t);
    const local_days dp = floor<days>(lt);
    const year_month_day ymd{dp};
    const hh_mm_ss<seconds> hms{lt - dp};
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u %02d:%02d",
                  static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()),
                  static_cast<unsigned>(ymd.day()),
                  static_cast<int>(hms.hours().count()),
                  static_cast<int>(hms.minutes().count()));
    return buf;
}

std::chrono::sys_seconds localMidnightUtc(std::chrono::sys_days date,
                                          const time_zone* zone) {
    const year_month_day ymd{date};
    return localCivilToUtc(static_cast<int>(ymd.year()),
                           static_cast<int>(static_cast<unsigned>(ymd.month())),
                           static_cast<int>(static_cast<unsigned>(ymd.day())), 0, 0,
                           zone);
}

std::chrono::sys_days localToday(const time_zone* zone) {
    // 로컬 civil 날짜의 day-count 를 그대로 sys_days(civil date)로 재해석(정책 A).
    const local_days ld = floor<days>(zone->to_local(system_clock::now()));
    return sys_days{ld.time_since_epoch()};
}

std::string progressBar(double ratio, int width) {
    int filled = static_cast<int>(ratio * width + 0.5);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    std::string bar(static_cast<std::size_t>(filled), '#');
    bar.append(static_cast<std::size_t>(width - filled), '-');
    return "[" + bar + "]";
}

domain::RecurrenceFrequency parseFrequency(const std::string& s) {
    if (s == "daily") return domain::RecurrenceFrequency::Daily;
    if (s == "weekly") return domain::RecurrenceFrequency::Weekly;
    if (s == "monthly") return domain::RecurrenceFrequency::Monthly;
    if (s == "yearly") return domain::RecurrenceFrequency::Yearly;
    throw std::runtime_error(
        "반복 주기는 daily|weekly|monthly|yearly 중 하나여야 합니다: " + s);
}

const char* frequencyText(domain::RecurrenceFrequency f) {
    switch (f) {
        case domain::RecurrenceFrequency::Daily: return "매일";
        case domain::RecurrenceFrequency::Weekly: return "매주";
        case domain::RecurrenceFrequency::Monthly: return "매월";
        case domain::RecurrenceFrequency::Yearly: return "매년";
    }
    return "?";
}

}  // namespace planning::ui
