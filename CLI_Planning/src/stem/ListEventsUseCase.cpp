#include "stem/ListEventsUseCase.hpp"

#include <algorithm>
#include <chrono>
#include <optional>

#include "seed/RecurrenceRule.hpp"
#include "seed/TimeRange.hpp"

namespace planning::application {

namespace {

using namespace std::chrono;

// 반복 주기만큼 다음 시작 시각으로 이동. 월/년은 무효일을 그 달 마지막 날로 클램프.
sys_seconds advance(sys_seconds t, domain::RecurrenceFrequency f) {
    switch (f) {
        case domain::RecurrenceFrequency::Daily:
            return t + days{1};
        case domain::RecurrenceFrequency::Weekly:
            return t + days{7};
        case domain::RecurrenceFrequency::Monthly:
        case domain::RecurrenceFrequency::Yearly: {
            const auto dp = floor<days>(t);
            const auto tod = t - dp;  // 하루 안의 시각
            const year_month_day ymd{dp};
            const year_month ym{ymd.year(), ymd.month()};
            const months step =
                (f == domain::RecurrenceFrequency::Monthly) ? months{1}
                                                            : months{12};
            const year_month nextYm = ym + step;
            year_month_day cand = nextYm.year() / nextYm.month() / ymd.day();
            if (!cand.ok()) {
                cand = nextYm.year() / nextYm.month() / last;  // 무효일 클램프
            }
            return sys_seconds{sys_days{cand}} + tod;
        }
    }
    return t;  // 도달 불가
}

// 반복 이벤트를 [winStart, winEnd) 범위 내 occurrence 들로 전개.
void expandRecurring(const domain::Event& e, sys_seconds winStart,
                     sys_seconds winEnd, std::vector<domain::Event>& out) {
    const auto rule = *e.recurrenceRule();
    const auto base = e.timeRange();
    const std::optional<seconds> duration =
        base.end() ? std::optional<seconds>{*base.end() - base.start()}
                   : std::nullopt;

    sys_seconds occStart = base.start();
    constexpr int kSafetyCap = 200000;
    for (int i = 0; i < kSafetyCap; ++i) {
        if (rule.until && occStart > *rule.until) break;
        if (occStart >= winEnd) break;
        if (occStart >= winStart) {
            const std::optional<sys_seconds> occEnd =
                duration ? std::optional<sys_seconds>{occStart + *duration}
                         : std::nullopt;
            out.emplace_back(e.id(), e.title(),
                             domain::TimeRange(occStart, occEnd, base.isAllDay()),
                             rule);
        }
        occStart = advance(occStart, rule.frequency);
    }
}

}  // namespace

ListEventsUseCase::ListEventsUseCase(ports::EventRepository& events,
                                     ports::Logger& logger)
    : events_(events), logger_(logger) {}

std::vector<domain::Event> ListEventsUseCase::execute(
    const ListEventsQuery& query) {
    const sys_seconds winStart{query.rangeStart};
    const sys_seconds winEnd{query.rangeEnd};
    const domain::TimeRange window(winStart, winEnd);

    std::vector<domain::Event> result;
    // NOTE(NF1): 현재는 findAll 후 인메모리 필터/전개. SQLite 어댑터 단계에서
    // 비반복은 findInRange 인덱스 조회 + 반복만 별도 조회로 최적화 예정.
    for (const auto& e : events_.findAll()) {
        if (e.recurrenceRule()) {
            expandRecurring(e, winStart, winEnd, result);
        } else if (e.timeRange().overlaps(window)) {
            result.push_back(e);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const domain::Event& a, const domain::Event& b) {
                  return a.timeRange().start() < b.timeRange().start();
              });

    logger_.audit("event.list", "listed");
    return result;
}

}  // namespace planning::application
