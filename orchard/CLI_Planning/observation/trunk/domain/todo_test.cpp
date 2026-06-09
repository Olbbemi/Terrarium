#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "trunk/domain/Priority.hpp"
#include "trunk/domain/Todo.hpp"

using planning::domain::Priority;
using planning::domain::Todo;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

bool hasTag(const Todo& t, const std::string& tag) {
    const auto& tags = t.tags();
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

}  // namespace

// --- happy ---

TEST(Todo, construct_with_optional_due) {
    Todo withDue(uuids::uuid{}, "장보기", Priority::MEDIUM, {"home", "errand"},
                 day(20000));
    EXPECT_EQ(withDue.title(), "장보기");
    EXPECT_FALSE(withDue.isDone());
    EXPECT_EQ(withDue.priority(), Priority::MEDIUM);
    EXPECT_EQ(withDue.tags().size(), 2u);
    ASSERT_TRUE(withDue.dueDate().has_value());
    EXPECT_EQ(*withDue.dueDate(), day(20000));

    Todo noDue(uuids::uuid{}, "운동", Priority::LOW, {});
    EXPECT_FALSE(noDue.dueDate().has_value());
}

TEST(Todo, mark_done) {
    Todo t(uuids::uuid{}, "보고서", Priority::HIGH, {});
    EXPECT_FALSE(t.isDone());
    t.markDone();
    EXPECT_TRUE(t.isDone());
}

TEST(Todo, add_tag) {
    Todo t(uuids::uuid{}, "청소", Priority::LOW, {});
    t.addTag("urgent");
    EXPECT_TRUE(hasTag(t, "urgent"));
}

TEST(Todo, remove_tag) {
    Todo t(uuids::uuid{}, "정리", Priority::LOW, {"a", "b"});
    t.removeTag("a");
    EXPECT_FALSE(hasTag(t, "a"));
    EXPECT_TRUE(hasTag(t, "b"));
}

// --- 실패 ---

TEST(Todo, throws_when_title_empty) {
    EXPECT_THROW(Todo(uuids::uuid{}, "", Priority::LOW, {}),
                 std::invalid_argument);
}

// --- edge ---

TEST(Todo, add_duplicate_tag_idempotent) {
    Todo t(uuids::uuid{}, "메모", Priority::LOW, {"x"});
    t.addTag("x");
    EXPECT_EQ(t.tags().size(), 1u);
}
