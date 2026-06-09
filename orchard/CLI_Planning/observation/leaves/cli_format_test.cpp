#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include "leaves/CliFormat.hpp"
#include "trunk/domain/Priority.hpp"
#include "trunk/domain/RecurrenceRule.hpp"

namespace pac = planning::ui;
using planning::domain::Priority;
using planning::domain::RecurrenceFrequency;

namespace {

std::chrono::sys_seconds sec(long s) {
    return std::chrono::sys_seconds{std::chrono::seconds{s}};
}

std::chrono::sys_days ymd(int y, unsigned m, unsigned d) {
    return std::chrono::sys_days{std::chrono::year{y} / std::chrono::month{m} /
                                 std::chrono::day{d}};
}

}  // namespace

// ---------- 타임존 비의존(pure) ----------

TEST(CliFormat, parseDate_valid) {
    EXPECT_EQ(pac::parseDate("2026-06-05"), ymd(2026, 6, 5));
}

TEST(CliFormat, parseDate_invalid_throws) {
    EXPECT_THROW(pac::parseDate("2026/06/05"), std::runtime_error);
    EXPECT_THROW(pac::parseDate("nope"), std::runtime_error);
}

TEST(CliFormat, formatDate_roundtrip) {
    EXPECT_EQ(pac::formatDate(ymd(2026, 6, 5)), "2026-06-05");
    EXPECT_EQ(pac::formatDate(ymd(2026, 12, 1)), "2026-12-01");
}

TEST(CliFormat, parsePriority_valid) {
    EXPECT_EQ(pac::parsePriority("high"), Priority::HIGH);
    EXPECT_EQ(pac::parsePriority("medium"), Priority::MEDIUM);
    EXPECT_EQ(pac::parsePriority("low"), Priority::LOW);
}

TEST(CliFormat, parsePriority_invalid_throws) {
    EXPECT_THROW(pac::parsePriority("urgent"), std::runtime_error);
}

TEST(CliFormat, priorityText_maps_back) {
    EXPECT_STREQ(pac::priorityText(Priority::HIGH), "high");
    EXPECT_STREQ(pac::priorityText(Priority::MEDIUM), "medium");
    EXPECT_STREQ(pac::priorityText(Priority::LOW), "low");
}

TEST(CliFormat, progressBar_fill_and_clamp) {
    EXPECT_EQ(pac::progressBar(0.0), "[----------]");
    EXPECT_EQ(pac::progressBar(0.3), "[###-------]");
    EXPECT_EQ(pac::progressBar(1.0), "[##########]");
    EXPECT_EQ(pac::progressBar(1.5), "[##########]");  // 초과 달성 클램프
    EXPECT_EQ(pac::progressBar(0.25, 4), "[#---]");
}

TEST(CliFormat, parseFrequency_valid) {
    EXPECT_EQ(pac::parseFrequency("daily"), RecurrenceFrequency::Daily);
    EXPECT_EQ(pac::parseFrequency("weekly"), RecurrenceFrequency::Weekly);
    EXPECT_EQ(pac::parseFrequency("monthly"), RecurrenceFrequency::Monthly);
    EXPECT_EQ(pac::parseFrequency("yearly"), RecurrenceFrequency::Yearly);
}

TEST(CliFormat, parseFrequency_invalid_throws) {
    EXPECT_THROW(pac::parseFrequency("hourly"), std::runtime_error);
}

TEST(CliFormat, frequencyText_maps) {
    EXPECT_STREQ(pac::frequencyText(RecurrenceFrequency::Daily), "매일");
    EXPECT_STREQ(pac::frequencyText(RecurrenceFrequency::Weekly), "매주");
    EXPECT_STREQ(pac::frequencyText(RecurrenceFrequency::Monthly), "매월");
    EXPECT_STREQ(pac::frequencyText(RecurrenceFrequency::Yearly), "매년");
}

// ---------- 타임존 의존: Asia/Seoul(KST, UTC+9) 명시 주입 ----------
// current_zone() 은 TZ 환경변수를 무시하므로, 결정적 검증을 위해 zone 을 주입한다.

namespace {
const std::chrono::time_zone* seoul() {
    return std::chrono::locate_zone("Asia/Seoul");
}
}  // namespace

TEST(CliFormat, parseDateTime_local_to_utc) {
    // 2026-06-05T14:00 KST == 2026-06-05 05:00 UTC == epoch 1780635600
    EXPECT_EQ(pac::parseDateTime("2026-06-05T14:00", seoul()), sec(1780635600));
}

TEST(CliFormat, parseDateTime_invalid_throws) {
    EXPECT_THROW(pac::parseDateTime("2026-06-05 14:00", seoul()),
                 std::runtime_error);
}

TEST(CliFormat, formatDateTime_utc_to_local) {
    EXPECT_EQ(pac::formatDateTime(sec(1780635600), seoul()), "2026-06-05 14:00");
}

TEST(CliFormat, localMidnightUtc_of_local_day) {
    // KST 자정(2026-06-05 00:00) == 2026-06-04 15:00 UTC == epoch 1780585200
    EXPECT_EQ(pac::localMidnightUtc(ymd(2026, 6, 5), seoul()), sec(1780585200));
}
