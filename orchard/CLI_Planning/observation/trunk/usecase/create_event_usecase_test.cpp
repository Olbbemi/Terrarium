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

// --- happy ---

TEST(CreateEventUseCase, no_conflict_persists_and_returns_createdId) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto result = uc.execute(baseCmd());

    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_EQ(repo.size(), 1u);
}

TEST(CreateEventUseCase, with_recurrence_sets_rule) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.recurrence = RecurrenceRule{RecurrenceFrequency::Weekly, std::nullopt};

    auto result = uc.execute(cmd);
    ASSERT_TRUE(result.createdId.has_value());
    auto saved = repo.findById(*result.createdId);
    ASSERT_TRUE(saved.has_value());
    ASSERT_TRUE(saved->recurrenceRule().has_value());
    EXPECT_EQ(saved->recurrenceRule()->frequency, RecurrenceFrequency::Weekly);
}

TEST(CreateEventUseCase, overlap_returns_conflict_unsaved) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(uuids::uuid{}, "기존회의", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.start = ts(150);
    cmd.end = ts(250);

    auto result = uc.execute(cmd);  // force=false
    EXPECT_TRUE(result.conflict.has_value());
    EXPECT_FALSE(result.createdId.has_value());
    EXPECT_EQ(repo.size(), 1u);          // 저장 안 됨
    EXPECT_TRUE(logger.auditLog.empty());  // 충돌 반환은 미기록
}

TEST(CreateEventUseCase, force_commits_despite_overlap) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(uuids::uuid{}, "기존회의", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.start = ts(150);
    cmd.end = ts(250);

    auto result = uc.execute(cmd, /*force=*/true);
    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_EQ(repo.size(), 2u);
}

// --- 실패 (불변식 = throw) ---

TEST(CreateEventUseCase, invalid_time_range_throws) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.start = ts(200);
    cmd.end = ts(100);

    EXPECT_THROW(uc.execute(cmd), std::invalid_argument);
}

TEST(CreateEventUseCase, empty_title_throws) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.title = "";

    EXPECT_THROW(uc.execute(cmd), std::invalid_argument);
    EXPECT_EQ(repo.size(), 0u);
}

// --- edge ---

TEST(CreateEventUseCase, all_day_no_end_persists) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

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

TEST(CreateEventUseCase, back_to_back_not_conflict) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(uuids::uuid{}, "이전회의", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto cmd = baseCmd();
    cmd.start = ts(200);  // 기존 종료시각과 맞닿음 → 겹치지 않음
    cmd.end = ts(300);

    auto result = uc.execute(cmd);  // force=false
    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_EQ(repo.size(), 2u);
}

TEST(CreateEventUseCase, force_with_no_overlap_persists_normally) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateEventUseCase uc(repo, detector, idGen, logger);

    auto result = uc.execute(baseCmd(), /*force=*/true);
    ASSERT_TRUE(result.createdId.has_value());
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_EQ(repo.size(), 1u);
}
