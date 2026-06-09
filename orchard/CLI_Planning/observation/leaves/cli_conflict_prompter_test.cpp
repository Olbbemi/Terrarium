#include <gtest/gtest.h>

#include <chrono>
#include <sstream>

#include "leaves/CliConflictPrompter.hpp"
#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/domain/TimeRange.hpp"

using planning::ui::CliConflictPrompter;
using Choice = planning::ports::ConflictPrompter::Choice;

namespace {

planning::domain::Conflict sampleConflict() {
    using namespace std::chrono;
    return planning::domain::Conflict{
        uuids::uuid{}, "회의",
        planning::domain::TimeRange(sys_seconds{seconds{100}},
                                    sys_seconds{seconds{200}})};
}

}  // namespace

TEST(CliConflictPrompter, yes_returns_add_anyway) {
    std::istringstream in("y\n");
    std::ostringstream out;
    CliConflictPrompter p(in, out);
    EXPECT_EQ(p.promptOnConflict(sampleConflict()), Choice::ADD_ANYWAY);
}

TEST(CliConflictPrompter, no_returns_cancel) {
    std::istringstream in("n\n");
    std::ostringstream out;
    CliConflictPrompter p(in, out);
    EXPECT_EQ(p.promptOnConflict(sampleConflict()), Choice::CANCEL);
}

TEST(CliConflictPrompter, eof_returns_cancel) {
    std::istringstream in("");  // 비대화/EOF
    std::ostringstream out;
    CliConflictPrompter p(in, out);
    EXPECT_EQ(p.promptOnConflict(sampleConflict()), Choice::CANCEL);
}
