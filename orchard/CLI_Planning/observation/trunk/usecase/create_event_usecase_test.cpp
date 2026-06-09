#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/RecurrenceRule.hpp"
#include "trunk/domain/TimeRange.hpp"
#include "trunk/usecase/CreateEventUseCase.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "support/fakes.hpp"

using planning::application::CreateEventCommand;
using planning::application::CreateEventUseCase;
using planning::domain::ConflictDetector;
using planning::domain::Event;
using planning::domain::RecurrenceFrequency;
using planning::domain::RecurrenceRule;
using planning::domain::TimeRange;
using Choice = planning::ports::ConflictPrompter::Choice;

namespace {

std::chrono::sys_seconds ts(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

CreateEventCommand baseCmd() {
    CreateEventCommand cmd;
    cmd.title = "회의";
    cmd.start = ts(100);
    cmd.end = ts(200);
    return cmd;
}

}  // namespace

TEST(CreateEventUseCase, persists_and_returns_id) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto result = uc.execute(baseCmd());

    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.cancelledByUser);
    EXPECT_EQ(repo.size(), 1u);
    EXPECT_EQ(prompter.callCount, 0);  // 충돌 없음 → 프롬프트 미호출
}

TEST(CreateEventUseCase, with_recurrence_sets_rule) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto cmd = baseCmd();
    cmd.recurrence = RecurrenceRule{RecurrenceFrequency::Weekly, std::nullopt};

    auto result = uc.execute(cmd);
    ASSERT_TRUE(result.createdId.has_value());
    auto saved = repo.findById(*result.createdId);
    ASSERT_TRUE(saved.has_value());
    ASSERT_TRUE(saved->recurrenceRule().has_value());
    EXPECT_EQ(saved->recurrenceRule()->frequency, RecurrenceFrequency::Weekly);
}

TEST(CreateEventUseCase, returns_id_on_user_accept_conflict) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(uuids::uuid{}, "기존회의", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto cmd = baseCmd();
    cmd.start = ts(150);
    cmd.end = ts(250);

    auto result = uc.execute(cmd);
    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.cancelledByUser);
    EXPECT_EQ(prompter.callCount, 1);
    EXPECT_EQ(repo.size(), 2u);
}

TEST(CreateEventUseCase, returns_cancelled_on_user_decline_conflict) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(uuids::uuid{}, "기존회의", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::CANCEL);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto cmd = baseCmd();
    cmd.start = ts(150);
    cmd.end = ts(250);

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.createdId.has_value());
    EXPECT_TRUE(result.cancelledByUser);
    EXPECT_EQ(prompter.callCount, 1);
    EXPECT_EQ(repo.size(), 1u);  // 저장 안 됨
}

TEST(CreateEventUseCase, all_day_no_end) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto cmd = baseCmd();
    cmd.end = std::nullopt;
    cmd.allDay = true;

    auto result = uc.execute(cmd);
    ASSERT_TRUE(result.createdId.has_value());
    auto saved = repo.findById(*result.createdId);
    ASSERT_TRUE(saved.has_value());
    EXPECT_TRUE(saved->timeRange().isAllDay());
    EXPECT_FALSE(saved->timeRange().end().has_value());
}

TEST(CreateEventUseCase, invalid_time_range_throws) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeConflictPrompter prompter(Choice::ADD_ANYWAY);
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, prompter, logger);

    auto cmd = baseCmd();
    cmd.start = ts(200);
    cmd.end = ts(100);

    EXPECT_THROW(uc.execute(cmd), std::invalid_argument);
}
