#include "roots/SqliteEventRepository.hpp"

#include <cstdint>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/Database.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/RecurrenceRule.hpp"
#include "trunk/domain/TimeRange.hpp"

namespace planning::store {

namespace {

using namespace std::chrono;

int64_t epochSec(sys_seconds t) {
    return static_cast<int64_t>(t.time_since_epoch().count());
}

sys_seconds toSysSeconds(int64_t sec) { return sys_seconds{seconds{sec}}; }

const char* freqToText(domain::RecurrenceFrequency f) {
    switch (f) {
        case domain::RecurrenceFrequency::Daily: return "daily";
        case domain::RecurrenceFrequency::Weekly: return "weekly";
        case domain::RecurrenceFrequency::Monthly: return "monthly";
        case domain::RecurrenceFrequency::Yearly: return "yearly";
    }
    return "daily";
}

domain::RecurrenceFrequency textToFreq(const std::string& s) {
    if (s == "weekly") return domain::RecurrenceFrequency::Weekly;
    if (s == "monthly") return domain::RecurrenceFrequency::Monthly;
    if (s == "yearly") return domain::RecurrenceFrequency::Yearly;
    return domain::RecurrenceFrequency::Daily;
}

// SELECT 컬럼 순서: id,title,start_ts,end_ts,all_day,recur_freq,recur_until
const char* kCols = "id, title, start_ts, end_ts, all_day, recur_freq, recur_until";

domain::Event rowToEvent(SQLite::Statement& q) {
    auto id = uuids::uuid::from_string(q.getColumn(0).getString()).value();
    std::string title = q.getColumn(1).getString();
    auto start = toSysSeconds(q.getColumn(2).getInt64());
    std::optional<sys_seconds> end;
    if (!q.getColumn(3).isNull()) end = toSysSeconds(q.getColumn(3).getInt64());
    const bool allDay = q.getColumn(4).getInt() != 0;
    domain::TimeRange range(start, end, allDay);

    std::optional<domain::RecurrenceRule> recurrence;
    if (!q.getColumn(5).isNull()) {
        domain::RecurrenceRule rule;
        rule.frequency = textToFreq(q.getColumn(5).getString());
        if (!q.getColumn(6).isNull()) {
            rule.until = toSysSeconds(q.getColumn(6).getInt64());
        }
        recurrence = rule;
    }
    return domain::Event(id, title, range, recurrence);
}

void bindBody(SQLite::Statement& s, const domain::Event& e) {
    s.bind(1, uuids::to_string(e.id()));
    s.bind(2, e.title());
    s.bind(3, epochSec(e.timeRange().start()));
    if (e.timeRange().end()) {
        s.bind(4, epochSec(*e.timeRange().end()));
    } else {
        s.bind(4);  // NULL
    }
    s.bind(5, e.timeRange().isAllDay() ? 1 : 0);
    if (e.recurrenceRule()) {
        s.bind(6, std::string(freqToText(e.recurrenceRule()->frequency)));
        if (e.recurrenceRule()->until) {
            s.bind(7, epochSec(*e.recurrenceRule()->until));
        } else {
            s.bind(7);
        }
    } else {
        s.bind(6);
        s.bind(7);
    }
}

}  // namespace

SqliteEventRepository::SqliteEventRepository(toolshed::sqlite::Database& db)
    : db_(db.handle()) {}

std::optional<domain::Event> SqliteEventRepository::findById(
    domain::Event::Id id) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM events WHERE id = ?");
    q.bind(1, uuids::to_string(id));
    if (q.executeStep()) return rowToEvent(q);
    return std::nullopt;
}

std::vector<domain::Event> SqliteEventRepository::findOverlapping(
    const domain::TimeRange& range) const {
    const int64_t candStart = epochSec(range.start());
    const int64_t candEnd = epochSec(range.end().value_or(range.start()));
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols +
                 " FROM events WHERE start_ts < ? AND "
                 "COALESCE(end_ts, start_ts) > ?");
    q.bind(1, candEnd);
    q.bind(2, candStart);
    std::vector<domain::Event> out;
    while (q.executeStep()) out.push_back(rowToEvent(q));
    return out;
}

std::vector<domain::Event> SqliteEventRepository::findInRange(
    std::chrono::sys_seconds start, std::chrono::sys_seconds end) const {
    const int64_t startSec = epochSec(start);
    const int64_t endSec = epochSec(end);
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols +
                 " FROM events WHERE start_ts < ? AND "
                 "COALESCE(end_ts, start_ts) > ?");
    q.bind(1, endSec);
    q.bind(2, startSec);
    std::vector<domain::Event> out;
    while (q.executeStep()) out.push_back(rowToEvent(q));
    return out;
}

std::vector<domain::Event> SqliteEventRepository::findAll() const {
    SQLite::Statement q(db_, std::string("SELECT ") + kCols + " FROM events");
    std::vector<domain::Event> out;
    while (q.executeStep()) out.push_back(rowToEvent(q));
    return out;
}

void SqliteEventRepository::save(const domain::Event& e) {
    SQLite::Statement s(
        db_,
        "INSERT INTO events (id, title, start_ts, end_ts, all_day, recur_freq, "
        "recur_until) VALUES (?, ?, ?, ?, ?, ?, ?)");
    bindBody(s, e);
    s.exec();
}

void SqliteEventRepository::update(const domain::Event& e) {
    SQLite::Statement s(
        db_,
        "UPDATE events SET title = ?, start_ts = ?, end_ts = ?, all_day = ?, "
        "recur_freq = ?, recur_until = ? WHERE id = ?");
    // bindBody 는 1..7 을 id,title,... 순으로 채우므로 UPDATE 용으로 직접 바인딩.
    s.bind(1, e.title());
    s.bind(2, epochSec(e.timeRange().start()));
    if (e.timeRange().end()) {
        s.bind(3, epochSec(*e.timeRange().end()));
    } else {
        s.bind(3);
    }
    s.bind(4, e.timeRange().isAllDay() ? 1 : 0);
    if (e.recurrenceRule()) {
        s.bind(5, std::string(freqToText(e.recurrenceRule()->frequency)));
        if (e.recurrenceRule()->until) {
            s.bind(6, epochSec(*e.recurrenceRule()->until));
        } else {
            s.bind(6);
        }
    } else {
        s.bind(5);
        s.bind(6);
    }
    s.bind(7, uuids::to_string(e.id()));
    s.exec();
}

void SqliteEventRepository::remove(domain::Event::Id id) {
    SQLite::Statement s(db_, "DELETE FROM events WHERE id = ?");
    s.bind(1, uuids::to_string(id));
    s.exec();
}

}  // namespace planning::store
