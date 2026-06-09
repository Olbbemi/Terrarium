#include <gtest/gtest.h>

#include <chrono>

#include "seed/Event.hpp"
#include "seed/Priority.hpp"
#include "seed/TimeRange.hpp"
#include "seed/Todo.hpp"
#include "stem/ShowDashboardUseCase.hpp"
#include "stem/commands/DashboardCommands.hpp"
#include "support/fakes.hpp"

using planning::application::ShowDashboardQuery;
using planning::application::ShowDashboardUseCase;
using planning::domain::Event;
using planning::domain::Priority;
using planning::domain::TimeRange;
using planning::domain::Todo;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

std::chrono::sys_seconds atDay(int n, int hour) {
    return std::chrono::sys_seconds{day(n)} + std::chrono::hours{hour};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

// 로컬 하루 창을 명시적으로 구성. todayDate(로컬 달력일, todo overdue 용) +
// [dayStart, dayEnd) UTC instant 경계(event 용).
ShowDashboardQuery query(std::chrono::sys_days todayDate,
                         std::chrono::sys_seconds dayStart,
                         std::chrono::sys_seconds dayEnd) {
    return ShowDashboardQuery{todayDate, dayStart, dayEnd};
}

// UTC 자정 정렬 하루 창 (todayDate = start 일자).
ShowDashboardQuery utcDay(int n) {
    return query(day(n), std::chrono::sys_seconds{day(n)},
                 std::chrono::sys_seconds{day(n + 1)});
}

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(ShowDashboardUseCase, empty_returns_zero_zero) {
    planning::test::FakeEventRepository events;
    planning::test::FakeTodoRepository todos;
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    auto r = uc.execute(utcDay(20000));
    EXPECT_EQ(r.todayEventsCount, 0);
    EXPECT_EQ(r.overdueTodosCount, 0);
}

TEST(ShowDashboardUseCase, returns_today_events_count) {
    planning::test::FakeEventRepository events;
    events.save(Event(id(k1), "오늘", TimeRange(atDay(20000, 10), atDay(20000, 11))));
    events.save(Event(id(k2), "내일", TimeRange(atDay(20001, 10), atDay(20001, 11))));
    planning::test::FakeTodoRepository todos;
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    auto r = uc.execute(utcDay(20000));
    EXPECT_EQ(r.todayEventsCount, 1);
}

// 회귀 가드: 로컬(예: KST) '오늘' 창은 UTC 자정을 가로지른다.
// [dayStart, dayEnd) 가 임의 instant 경계로 들어오면 그 안의 이벤트를 세야 한다.
// (옛 sys_days 창 [day20000, day20001) 였다면 이 이벤트는 0개로 누락됐다 — 원래 버그.)
TEST(ShowDashboardUseCase, counts_event_in_local_day_window_crossing_utc_midnight) {
    planning::test::FakeEventRepository events;
    // UTC 로는 '어제' 23:00 이지만 로컬 오늘 저녁에 해당하는 이벤트.
    events.save(Event(id(k1), "로컬오늘저녁",
                      TimeRange(atDay(19999, 23), atDay(20000, 0))));
    planning::test::FakeTodoRepository todos;
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    // 로컬 자정(오늘) = UTC 어제 15:00, 로컬 자정(내일) = UTC 오늘 15:00.
    auto r = uc.execute(query(day(20000),
                              std::chrono::sys_seconds{day(19999)} + std::chrono::hours{15},
                              std::chrono::sys_seconds{day(20000)} + std::chrono::hours{15}));
    EXPECT_EQ(r.todayEventsCount, 1);
}

TEST(ShowDashboardUseCase, returns_overdue_todos_count) {
    planning::test::FakeEventRepository events;
    planning::test::FakeTodoRepository todos;
    todos.save(Todo(id(k1), "지남", Priority::LOW, {}, day(19990)));   // overdue
    todos.save(Todo(id(k2), "미래", Priority::LOW, {}, day(20010)));   // not yet
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    auto r = uc.execute(utcDay(20000));
    EXPECT_EQ(r.overdueTodosCount, 1);
}
