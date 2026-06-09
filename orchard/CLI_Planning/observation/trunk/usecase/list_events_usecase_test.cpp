#include <gtest/gtest.h>

#include <chrono>
#include <optional>

#include "trunk/domain/Event.hpp"
#include "trunk/domain/RecurrenceRule.hpp"
#include "trunk/domain/TimeRange.hpp"
#include "trunk/usecase/ListEventsUseCase.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "support/fakes.hpp"

using planning::application::ListEventsQuery;
using planning::application::ListEventsUseCase;
using planning::domain::Event;
using planning::domain::RecurrenceFrequency;
using planning::domain::RecurrenceRule;
using planning::domain::TimeRange;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

std::chrono::sys_seconds atDay(int n, int hour) {
    return std::chrono::sys_seconds{day(n)} + std::chrono::hours{hour};
}

Event ev(const char* uuid, const char* title, int dayN) {
    return Event(uuids::uuid::from_string(uuid).value(), title,
                 TimeRange(atDay(dayN, 10), atDay(dayN, 11)));
}

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";
const char* k3 = "33333333-3333-3333-3333-333333333333";

}  // namespace

TEST(ListEventsUseCase, empty_returns_empty) {
    planning::test::FakeEventRepository repo;
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);
    auto result = uc.execute(ListEventsQuery{day(20000), day(20007)});
    EXPECT_TRUE(result.empty());
}

TEST(ListEventsUseCase, filter_today) {
    planning::test::FakeEventRepository repo;
    repo.save(ev(k1, "오늘", 20000));
    repo.save(ev(k2, "나중", 20005));
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);

    auto result = uc.execute(ListEventsQuery{day(20000), day(20001)});
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "오늘");
}

// 회귀 가드: 로컬(예: KST) '오늘' 창은 UTC 자정을 가로지른다.
// 범위 경계가 임의 instant(sys_seconds)로 들어오면 그 창의 이벤트를 전개/포함해야 한다.
TEST(ListEventsUseCase, filters_by_instant_window_crossing_utc_midnight) {
    planning::test::FakeEventRepository repo;
    // UTC 로는 '어제' 23:00 이지만 로컬 오늘 저녁에 해당하는 이벤트.
    repo.save(Event(uuids::uuid::from_string(k1).value(), "로컬오늘저녁",
                    TimeRange(atDay(19999, 23), atDay(20000, 0))));
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);

    // 로컬 자정(오늘)=UTC 어제 15:00, 로컬 자정(내일)=UTC 오늘 15:00.
    auto result = uc.execute(ListEventsQuery{atDay(19999, 15), atDay(20000, 15)});
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "로컬오늘저녁");
}

TEST(ListEventsUseCase, filter_week) {
    planning::test::FakeEventRepository repo;
    repo.save(ev(k1, "A", 20000));
    repo.save(ev(k2, "B", 20005));
    repo.save(ev(k3, "C", 20010));
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);

    auto result = uc.execute(ListEventsQuery{day(20000), day(20007)});
    EXPECT_EQ(result.size(), 2u);  // A, B (C는 범위 밖)
}

TEST(ListEventsUseCase, filter_range) {
    planning::test::FakeEventRepository repo;
    repo.save(ev(k1, "A", 20000));
    repo.save(ev(k2, "B", 20005));
    repo.save(ev(k3, "C", 20010));
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);

    auto result = uc.execute(ListEventsQuery{day(20003), day(20006)});
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "B");
}

TEST(ListEventsUseCase, includes_recurring_instances) {
    planning::test::FakeEventRepository repo;
    Event weekly(uuids::uuid::from_string(k1).value(), "주간회의",
                 TimeRange(atDay(20000, 10), atDay(20000, 11)),
                 RecurrenceRule{RecurrenceFrequency::Weekly, std::nullopt});
    repo.save(weekly);
    planning::test::FakeLogger logger;
    ListEventsUseCase uc(repo, logger);

    // [20000, 20021) 3주 → 20000, 20007, 20014 (20021 제외)
    auto result = uc.execute(ListEventsQuery{day(20000), day(20021)});
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].timeRange().start(), atDay(20000, 10));
    EXPECT_EQ(result[1].timeRange().start(), atDay(20007, 10));
    EXPECT_EQ(result[2].timeRange().start(), atDay(20014, 10));
}
