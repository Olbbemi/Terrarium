#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

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

namespace {

std::chrono::sys_seconds ts(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "11111111-1111-1111-1111-111111111111";
const char* kB = "22222222-2222-2222-2222-222222222222";

}  // namespace

// --- happy ---

TEST(UpdateEventUseCase, non_time_change_persists_without_recheck) {
    planning::test::FakeEventRepository repo;
    // A 와 B 는 시간이 겹치지만, B 의 제목만 바꾸면 재검사하지 않아야 한다.
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(150), ts(250))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.title = "B2";  // 시간 미변경

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_TRUE(result.updated);
    EXPECT_EQ(repo.findById(id(kB))->title(), "B2");
}

TEST(UpdateEventUseCase, time_change_no_overlap_persists) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(150);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(250)};

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_TRUE(result.updated);
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(150));
}

TEST(UpdateEventUseCase, time_change_overlap_returns_conflict_unsaved) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(150);  // A 와 겹치도록 이동
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(250)};

    auto result = uc.execute(cmd);  // force=false
    EXPECT_TRUE(result.conflict.has_value());
    EXPECT_FALSE(result.updated);
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(300));  // 변경 안 됨
    EXPECT_TRUE(logger.auditLog.empty());  // 충돌 반환은 미기록
}

TEST(UpdateEventUseCase, force_commits_despite_overlap) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(150);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(250)};

    auto result = uc.execute(cmd, /*force=*/true);
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_TRUE(result.updated);
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(150));
}

// --- 실패 (불변식 = throw) ---

TEST(UpdateEventUseCase, throws_when_not_found) {
    planning::test::FakeEventRepository repo;
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.title = "없음";

    EXPECT_THROW(uc.execute(cmd), std::out_of_range);
}

TEST(UpdateEventUseCase, invalid_time_range_throws) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(200);
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(100)};

    EXPECT_THROW(uc.execute(cmd), std::invalid_argument);
}

// --- edge ---

TEST(UpdateEventUseCase, partial_update_only_changed_fields) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "원제목", TimeRange(ts(100), ts(200))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.title = "새제목";  // 제목만 변경

    auto result = uc.execute(cmd);
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_TRUE(result.updated);
    auto updated = repo.findById(id(kB));
    EXPECT_EQ(updated->title(), "새제목");
    EXPECT_EQ(updated->timeRange().start(), ts(100));  // 시간 유지
}

TEST(UpdateEventUseCase, back_to_back_after_move_not_conflict) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kA), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(kB), "B", TimeRange(ts(300), ts(400))));
    ConflictDetector detector;
    planning::test::FakeLogger logger;
    UpdateEventUseCase uc(repo, detector, logger);

    UpdateEventCommand cmd;
    cmd.id = id(kB);
    cmd.start = ts(200);  // A 의 종료시각과 맞닿음 → 겹치지 않음
    cmd.end = std::optional<std::chrono::sys_seconds>{ts(300)};

    auto result = uc.execute(cmd);  // force=false
    EXPECT_FALSE(result.conflict.has_value());
    EXPECT_TRUE(result.updated);
    EXPECT_EQ(repo.findById(id(kB))->timeRange().start(), ts(200));
}
