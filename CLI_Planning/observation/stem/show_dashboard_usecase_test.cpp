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

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(ShowDashboardUseCase, empty_returns_zero_zero) {
    planning::test::FakeEventRepository events;
    planning::test::FakeTodoRepository todos;
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    auto r = uc.execute(ShowDashboardQuery{day(20000)});
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

    auto r = uc.execute(ShowDashboardQuery{day(20000)});
    EXPECT_EQ(r.todayEventsCount, 1);
}

TEST(ShowDashboardUseCase, returns_overdue_todos_count) {
    planning::test::FakeEventRepository events;
    planning::test::FakeTodoRepository todos;
    todos.save(Todo(id(k1), "지남", Priority::LOW, {}, day(19990)));   // overdue
    todos.save(Todo(id(k2), "미래", Priority::LOW, {}, day(20010)));   // not yet
    planning::test::FakeLogger logger;
    ShowDashboardUseCase uc(events, todos, logger);

    auto r = uc.execute(ShowDashboardQuery{day(20000)});
    EXPECT_EQ(r.overdueTodosCount, 1);
}
