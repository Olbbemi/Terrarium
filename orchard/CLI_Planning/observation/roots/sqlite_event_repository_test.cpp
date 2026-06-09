#include <gtest/gtest.h>

#include <chrono>
#include <optional>

#include "roots/SqliteEventRepository.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/RecurrenceRule.hpp"
#include "trunk/domain/TimeRange.hpp"
#include "support/sqlite_test_db.hpp"

using planning::adapter_sqlite::SqliteEventRepository;
using planning::domain::Event;
using planning::domain::RecurrenceFrequency;
using planning::domain::RecurrenceRule;
using planning::domain::TimeRange;
using planning::test::makeMigratedDatabase;

namespace {

std::chrono::sys_seconds ts(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

std::chrono::sys_seconds atDay(int n, int hour) {
    return std::chrono::sys_seconds{day(n)} + std::chrono::hours{hour};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";
const char* k3 = "33333333-3333-3333-3333-333333333333";

}  // namespace

TEST(SqliteEventRepository, save_findById_roundtrip) {
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);

    Event e(id(k1), "주간회의", TimeRange(ts(1000), ts(4600)),
            RecurrenceRule{RecurrenceFrequency::Weekly, ts(99999)});
    repo.save(e);

    auto loaded = repo.findById(id(k1));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->title(), "주간회의");
    EXPECT_EQ(loaded->timeRange().start(), ts(1000));
    ASSERT_TRUE(loaded->timeRange().end().has_value());
    EXPECT_EQ(*loaded->timeRange().end(), ts(4600));
    ASSERT_TRUE(loaded->recurrenceRule().has_value());
    EXPECT_EQ(loaded->recurrenceRule()->frequency, RecurrenceFrequency::Weekly);
    ASSERT_TRUE(loaded->recurrenceRule()->until.has_value());
    EXPECT_EQ(*loaded->recurrenceRule()->until, ts(99999));
}

TEST(SqliteEventRepository, persists_all_day_flag_through_update) {
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);

    repo.save(Event(id(k1), "종일", TimeRange(ts(1000), ts(4600), true)));
    auto loaded = repo.findById(id(k1));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->timeRange().isAllDay());

    // 하루 종일 해제로 update 후에도 라운드트립 유지.
    Event toggled(id(k1), "종일", TimeRange(ts(1000), ts(4600), false));
    repo.update(toggled);
    auto reloaded = repo.findById(id(k1));
    ASSERT_TRUE(reloaded.has_value());
    EXPECT_FALSE(reloaded->timeRange().isAllDay());
}

TEST(SqliteEventRepository, findById_returns_nullopt_when_not_found) {
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);
    EXPECT_FALSE(repo.findById(id(k1)).has_value());
}

TEST(SqliteEventRepository, findOverlapping) {
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);
    repo.save(Event(id(k1), "A", TimeRange(ts(100), ts(200))));
    repo.save(Event(id(k2), "B", TimeRange(ts(150), ts(250))));
    repo.save(Event(id(k3), "C", TimeRange(ts(300), ts(400))));

    auto result = repo.findOverlapping(TimeRange(ts(120), ts(160)));
    // [120,160) 과 겹치는 건 A[100,200), B[150,250)
    EXPECT_EQ(result.size(), 2u);
}

TEST(SqliteEventRepository, findInRange_filters) {
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);
    repo.save(Event(id(k1), "오늘", TimeRange(atDay(20000, 10), atDay(20000, 11))));
    repo.save(Event(id(k2), "사흘후", TimeRange(atDay(20003, 10), atDay(20003, 11))));

    auto result = repo.findInRange(day(20000), day(20001));
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "오늘");
}

TEST(SqliteEventRepository, findInRange_uses_instant_boundaries) {
    // 경계가 '일 자정' 정렬이 아니라 임의 instant 여도 정확히 걸러야 한다.
    // (로컬 자정은 UTC 로 비정렬 instant — 이 슬라이스의 핵심.)
    auto db = makeMigratedDatabase();
    SqliteEventRepository repo(db);
    repo.save(Event(id(k1), "범위안", TimeRange(atDay(20000, 5), atDay(20000, 6))));
    repo.save(Event(id(k2), "범위밖", TimeRange(atDay(20000, 20), atDay(20000, 21))));

    // [04:00, 10:00) UTC 창
    auto result = repo.findInRange(atDay(20000, 4), atDay(20000, 10));
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "범위안");
}
