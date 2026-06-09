#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

#include "seed/Event.hpp"
#include "seed/RecurrenceRule.hpp"
#include "seed/TimeRange.hpp"
#include "stem/DeleteEventUseCase.hpp"
#include "stem/commands/EventCommands.hpp"
#include "support/fakes.hpp"

using planning::application::DeleteEventCommand;
using planning::application::DeleteEventUseCase;
using planning::domain::Event;
using planning::domain::RecurrenceFrequency;
using planning::domain::RecurrenceRule;
using planning::domain::TimeRange;

namespace {

std::chrono::sys_seconds ts(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "11111111-1111-1111-1111-111111111111";
const char* kB = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(DeleteEventUseCase, removes_event) {
    planning::test::FakeEventRepository repo;
    repo.save(Event(id(kB), "B", TimeRange(ts(100), ts(200))));
    planning::test::FakeLogger logger;
    DeleteEventUseCase uc(repo, logger);

    uc.execute(DeleteEventCommand{id(kB)});
    EXPECT_FALSE(repo.findById(id(kB)).has_value());
    EXPECT_EQ(repo.size(), 0u);
}

TEST(DeleteEventUseCase, throws_when_not_found) {
    planning::test::FakeEventRepository repo;
    planning::test::FakeLogger logger;
    DeleteEventUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(DeleteEventCommand{id(kA)}), std::out_of_range);
}

TEST(DeleteEventUseCase, delete_recurring_removes_all_instances) {
    planning::test::FakeEventRepository repo;
    // 반복 이벤트는 규칙을 가진 단일 레코드로 저장되며 인스턴스는 조회 시 전개된다.
    // 따라서 레코드 삭제가 곧 모든 인스턴스 삭제.
    Event recurring(id(kB), "주간회의", TimeRange(ts(100), ts(200)),
                    RecurrenceRule{RecurrenceFrequency::Weekly, std::nullopt});
    repo.save(recurring);
    planning::test::FakeLogger logger;
    DeleteEventUseCase uc(repo, logger);

    uc.execute(DeleteEventCommand{id(kB)});
    EXPECT_EQ(repo.size(), 0u);
}
