#include <gtest/gtest.h>

#include <chrono>

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/TimeRange.hpp"
#include "trunk/usecase/UpdateEventUseCase.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "support/fakes.hpp"

using planning::application::UpdateEventCommand;
using planning::application::UpdateEventUseCase;
using planning::domain::ConflictDetector;
using planning::domain::Event;
using planning::domain::TimeRange;
using Choice = planning::ports::ConflictPrompter::Choice;

namespace {

std::chrono::sys_seconds ts(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "11111111-1111-1111-1111-111111111111";
const char* kB = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(UpdateEventUseCase, persists_changes) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.title = "B2";
    cmd.start = ts(310);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(420)};

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.cancelledByUser);
    auto updated = repo.findById(id(kB));
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->title(), "B2");
    EXPECT_EQ(updated->timeRange().start(), ts(310));
}

TEST(UpdateEventUseCase, re_checks_conflict) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(150);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(250)};

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.cancelledByUser);
    EXPECT_EQ(prompter.callCount, 1);  // 시간 변경 → A 와 충돌 재검사
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(150));
}

TEST(UpdateEventUseCase, returns_cancelled_on_user_decline_after_change) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::CANCEL);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(150);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(250)};

    auto result = uc.execute(cmd);
    EXPECT_TRUE(result.cancelledByUser);
    EXPECT_EQ(prompter.callCount, 1);
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(300));  // 변경 안 됨
}

TEST(UpdateEventUseCase, clears_end_time) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    // 종료시각 해제: 바깥 optional 있음(변경) + 안쪽 nullopt(값 없음).
    cmd.end = std::optional<std::chrono::sys_seconds>{std::nullopt};

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.cancelledByUser);
    auto updated = repo.findById(id(kB));
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->timeRange().start(), ts(100));     // 시작 유지
    EXPECT_FALSE(updated->timeRange().end().has_value());  // 종료 제거됨
}

TEST(UpdateEventUseCase, toggles_all_day) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(100), ts(200), false)));
    ConflictDetector detector;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.allDay = true;

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.cancelledByUser);
    auto updated = repo.findById(id(kB));
    ASSERT_TRUE(updated.has_value());
    EXPECT_TRUE(updated->timeRange().isAllDay());
    EXPECT_EQ(updated->timeRange().start(), ts(100));  // 시간은 유지
}

TEST(UpdateEventUseCase, partial_update_only_changed_fields) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "원제목", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, prompter, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.title = "새제목";  // 제목만 변경

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.cancelledByUser);
    auto updated = repo.findById(id(kB));
    EXPECT_EQ(updated->title(), "새제목");
    EXPECT_EQ(updated->timeRange().start(), ts(100));  // 시간 유지
    EXPECT_EQ(prompter.callCount, 0);  // 시간 미변경 → 충돌 재검사 안 함
}
