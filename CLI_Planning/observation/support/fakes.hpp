#pragma once

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "seed/IdGenerator.hpp"
#include "stomata/ConflictPrompter.hpp"
#include "stomata/EventRepository.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"
#include "stomata/TodoRepository.hpp"

namespace planning::test {

// 카운터 기반으로 유일한 UUID 를 차례로 발급하는 가짜 생성기.
class FakeIdGenerator : public domain::IdGenerator {
public:
    uuids::uuid next() override {
        char buf[37];
        std::snprintf(buf, sizeof(buf), "00000000-0000-0000-0000-%012x",
                      counter_++);
        return uuids::uuid::from_string(buf).value();
    }

private:
    unsigned counter_ = 1;
};

// 호출을 기록만 하는 가짜 로거. audit 호출은 검증용으로 보관.
class FakeLogger : public ports::Logger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
    void audit(const std::string& action, const std::string& detail) override {
        auditLog.emplace_back(action, detail);
    }

    std::vector<std::pair<std::string, std::string>> auditLog;
};

// 미리 정한 선택을 반환하는 가짜 충돌 프롬프터. 호출 횟수 기록.
class FakeConflictPrompter : public ports::ConflictPrompter {
public:
    explicit FakeConflictPrompter(Choice choice) : choice_(choice) {}

    Choice promptOnConflict(const domain::Conflict&) override {
        ++callCount;
        return choice_;
    }

    int callCount = 0;

private:
    Choice choice_;
};

// 벡터 기반 in-memory EventRepository.
class FakeEventRepository : public ports::EventRepository {
public:
    std::optional<domain::Event> findById(domain::Event::Id id) const override {
        for (const auto& e : events_) {
            if (e.id() == id) return e;
        }
        return std::nullopt;
    }

    std::vector<domain::Event> findOverlapping(
        const domain::TimeRange& range) const override {
        std::vector<domain::Event> out;
        for (const auto& e : events_) {
            if (e.timeRange().overlaps(range)) out.push_back(e);
        }
        return out;
    }

    std::vector<domain::Event> findInRange(
        std::chrono::sys_days start, std::chrono::sys_days end) const override {
        domain::TimeRange window(std::chrono::sys_seconds{start},
                                 std::chrono::sys_seconds{end});
        std::vector<domain::Event> out;
        for (const auto& e : events_) {
            if (e.timeRange().overlaps(window)) out.push_back(e);
        }
        return out;
    }

    std::vector<domain::Event> findAll() const override { return events_; }

    void save(const domain::Event& e) override { events_.push_back(e); }

    void update(const domain::Event& e) override {
        for (auto& cur : events_) {
            if (cur.id() == e.id()) {
                cur = e;
                return;
            }
        }
    }

    void remove(domain::Event::Id id) override {
        events_.erase(
            std::remove_if(events_.begin(), events_.end(),
                           [&](const domain::Event& e) { return e.id() == id; }),
            events_.end());
    }

    std::size_t size() const { return events_.size(); }

private:
    std::vector<domain::Event> events_;
};

// 벡터 기반 in-memory TodoRepository.
class FakeTodoRepository : public ports::TodoRepository {
public:
    std::optional<domain::Todo> findById(domain::Todo::Id id) const override {
        for (const auto& t : todos_) {
            if (t.id() == id) return t;
        }
        return std::nullopt;
    }

    std::vector<domain::Todo> findByDueDate(
        std::chrono::sys_days due) const override {
        std::vector<domain::Todo> out;
        for (const auto& t : todos_) {
            if (t.dueDate() && *t.dueDate() == due) out.push_back(t);
        }
        return out;
    }

    std::vector<domain::Todo> findOverdue(
        std::chrono::sys_days today) const override {
        std::vector<domain::Todo> out;
        for (const auto& t : todos_) {
            if (!t.isDone() && t.dueDate() && *t.dueDate() < today) {
                out.push_back(t);
            }
        }
        return out;
    }

    std::vector<domain::Todo> findByTag(const std::string& tag) const override {
        std::vector<domain::Todo> out;
        for (const auto& t : todos_) {
            const auto& tags = t.tags();
            if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
                out.push_back(t);
            }
        }
        return out;
    }

    std::vector<domain::Todo> findByPriority(
        domain::Priority p) const override {
        std::vector<domain::Todo> out;
        for (const auto& t : todos_) {
            if (t.priority() == p) out.push_back(t);
        }
        return out;
    }

    std::vector<domain::Todo> findAll() const override { return todos_; }

    void save(const domain::Todo& t) override { todos_.push_back(t); }

    void update(const domain::Todo& t) override {
        for (auto& cur : todos_) {
            if (cur.id() == t.id()) {
                cur = t;
                return;
            }
        }
    }

    void remove(domain::Todo::Id id) override {
        todos_.erase(
            std::remove_if(todos_.begin(), todos_.end(),
                           [&](const domain::Todo& t) { return t.id() == id; }),
            todos_.end());
    }

    std::size_t size() const { return todos_.size(); }

private:
    std::vector<domain::Todo> todos_;
};

// 벡터 기반 in-memory GoalRepository.
class FakeGoalRepository : public ports::GoalRepository {
public:
    std::optional<domain::Goal> findById(domain::Goal::Id id) const override {
        for (const auto& g : goals_) {
            if (g.id() == id) return g;
        }
        return std::nullopt;
    }

    std::optional<domain::Goal> findByName(
        const std::string& name) const override {
        for (const auto& g : goals_) {
            if (g.name() == name) return g;
        }
        return std::nullopt;
    }

    std::vector<domain::Goal> findAll() const override { return goals_; }

    void save(const domain::Goal& g) override { goals_.push_back(g); }

    void update(const domain::Goal& g) override {
        for (auto& cur : goals_) {
            if (cur.id() == g.id()) {
                cur = g;
                return;
            }
        }
    }

    void remove(domain::Goal::Id id) override {
        goals_.erase(
            std::remove_if(goals_.begin(), goals_.end(),
                           [&](const domain::Goal& g) { return g.id() == id; }),
            goals_.end());
    }

    std::size_t size() const { return goals_.size(); }

private:
    std::vector<domain::Goal> goals_;
};

}  // namespace planning::test
