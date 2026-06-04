#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include "climate/TomlConfigLoader.hpp"
#include "leaves/CliConflictPrompter.hpp"
#include "rings/SpdlogLogger.hpp"
#include "roots/MigrationRunner.hpp"
#include "roots/SqliteEventRepository.hpp"
#include "roots/SqliteTodoRepository.hpp"
#include "seed/ConflictDetector.hpp"
#include "seed/Priority.hpp"
#include "seed/StdUuidGenerator.hpp"
#include "seed/Todo.hpp"
#include "stem/AddTodoUseCase.hpp"
#include "stem/CreateEventUseCase.hpp"
#include "stem/DeleteEventUseCase.hpp"
#include "stem/ListEventsUseCase.hpp"
#include "stem/ListTodosUseCase.hpp"
#include "stem/MarkTodoDoneUseCase.hpp"
#include "stem/ShowDashboardUseCase.hpp"
#include "stem/UpdateEventUseCase.hpp"
#include "stem/commands/DashboardCommands.hpp"
#include "stem/commands/EventCommands.hpp"
#include "stem/commands/TodoCommands.hpp"

namespace {

namespace pa = planning::application;

// 로컬 civil(broken-down) 시각 → UTC sys_seconds.
// mktime 이 시스템 로컬 타임존 기준으로 해석해 epoch 초로 변환한다.
// (GCC 11 libstdc++ 는 std::chrono::current_zone 미지원 → POSIX <ctime> 사용.)
std::chrono::sys_seconds localCivilToUtc(int y, int mo, int d, int h, int mi) {
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;  // DST 여부 자동 판단
    const std::time_t t = std::mktime(&tm);
    return std::chrono::sys_seconds{
        std::chrono::seconds{static_cast<std::int64_t>(t)}};
}

// "YYYY-MM-DDTHH:MM" 를 로컬 시각으로 해석해 UTC sys_seconds 로 저장(정책 A).
std::chrono::sys_seconds parseDateTime(const std::string& s) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) != 5) {
        throw std::runtime_error("잘못된 날짜시간 형식 (YYYY-MM-DDTHH:MM): " + s);
    }
    return localCivilToUtc(y, mo, d, h, mi);
}

// "YYYY-MM-DD" → 달력 날짜(sys_days). 마감일은 시각 없는 civil date 라 변환 불필요.
std::chrono::sys_days parseDate(const std::string& s) {
    int y = 0, mo = 0, d = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &d) != 3) {
        throw std::runtime_error("잘못된 날짜 형식 (YYYY-MM-DD): " + s);
    }
    return std::chrono::sys_days{std::chrono::year{y} /
                                 std::chrono::month{static_cast<unsigned>(mo)} /
                                 std::chrono::day{static_cast<unsigned>(d)}};
}

// UTC sys_seconds → 로컬 "YYYY-MM-DD HH:MM" 표시(정책 A).
std::string formatDateTime(std::chrono::sys_seconds t) {
    const std::time_t tt = static_cast<std::time_t>(t.time_since_epoch().count());
    std::tm lt{};
    ::localtime_r(&tt, &lt);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d", lt.tm_year + 1900,
                  lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min);
    return buf;
}

// 로컬 달력 날짜의 자정 → UTC instant (event 범위 경계용).
std::chrono::sys_seconds localMidnightUtc(std::chrono::sys_days date) {
    const std::chrono::year_month_day ymd{date};
    return localCivilToUtc(static_cast<int>(ymd.year()),
                           static_cast<int>(static_cast<unsigned>(ymd.month())),
                           static_cast<int>(static_cast<unsigned>(ymd.day())), 0, 0);
}

std::string formatDate(std::chrono::sys_days d) {
    const std::chrono::year_month_day ymd{d};
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()),
                  static_cast<unsigned>(ymd.day()));
    return buf;
}

planning::domain::Priority parsePriority(const std::string& s) {
    if (s == "high") return planning::domain::Priority::HIGH;
    if (s == "medium") return planning::domain::Priority::MEDIUM;
    if (s == "low") return planning::domain::Priority::LOW;
    throw std::runtime_error("우선순위는 high|medium|low 중 하나여야 합니다: " + s);
}

const char* priorityText(planning::domain::Priority p) {
    switch (p) {
        case planning::domain::Priority::HIGH: return "high";
        case planning::domain::Priority::MEDIUM: return "medium";
        case planning::domain::Priority::LOW: return "low";
    }
    return "?";
}

// 시스템 로컬 타임존 기준 오늘 달력 날짜(정책 A).
std::chrono::sys_days localTodayDate() {
    const std::time_t nowT = std::time(nullptr);
    std::tm lt{};
    ::localtime_r(&nowT, &lt);
    return std::chrono::sys_days{
        std::chrono::year{lt.tm_year + 1900} /
        std::chrono::month{static_cast<unsigned>(lt.tm_mon + 1)} /
        std::chrono::day{static_cast<unsigned>(lt.tm_mday)}};
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Terrarium (gaia) — CLI 일정 관리 (vertical slice)"};
    app.require_subcommand(0);  // 서브커맨드 없으면 대시보드

    std::string configPath;
    app.add_option("--config", configPath, "TOML 설정 파일 경로")->required();

    auto* event = app.add_subcommand("event", "이벤트 관리");
    auto* eventAdd = event->add_subcommand("add", "이벤트 추가");
    std::string title, startStr, endStr;
    eventAdd->add_option("--title", title, "제목")->required();
    eventAdd->add_option("--start", startStr, "시작 (YYYY-MM-DDTHH:MM)")->required();
    eventAdd->add_option("--end", endStr, "종료 (YYYY-MM-DDTHH:MM)");

    auto* eventList = event->add_subcommand("list", "이벤트 목록 (기본: 오늘)");
    bool evWeek = false;
    std::string evFrom, evTo;
    eventList->add_flag("--week", evWeek, "오늘부터 7일");
    eventList->add_option("--from", evFrom, "시작일 (YYYY-MM-DD, 포함)");
    eventList->add_option("--to", evTo, "종료일 (YYYY-MM-DD, 포함)");

    auto* eventUpdate = event->add_subcommand("update", "이벤트 수정 (부분)");
    std::string evUpId, evUpTitle, evUpStart, evUpEnd;
    eventUpdate->add_option("--id", evUpId, "이벤트 ID (uuid)")->required();
    auto* oUpTitle = eventUpdate->add_option("--title", evUpTitle, "제목");
    auto* oUpStart =
        eventUpdate->add_option("--start", evUpStart, "시작 (YYYY-MM-DDTHH:MM)");
    auto* oUpEnd =
        eventUpdate->add_option("--end", evUpEnd, "종료 (YYYY-MM-DDTHH:MM)");

    auto* eventDelete = event->add_subcommand("delete", "이벤트 삭제");
    std::string evDelId;
    eventDelete->add_option("--id", evDelId, "이벤트 ID (uuid)")->required();

    auto* todo = app.add_subcommand("todo", "할 일 관리");

    auto* todoAdd = todo->add_subcommand("add", "할 일 추가");
    std::string todoTitle, todoPriority = "medium", todoDue;
    std::vector<std::string> todoTags;
    todoAdd->add_option("--title", todoTitle, "제목")->required();
    todoAdd->add_option("--priority", todoPriority,
                        "우선순위 high|medium|low (기본 medium)");
    todoAdd->add_option("--tag", todoTags, "태그 (반복 지정 가능)");
    todoAdd->add_option("--due", todoDue, "마감일 (YYYY-MM-DD)");

    auto* todoList = todo->add_subcommand("list", "할 일 목록");
    bool listToday = false, listOverdue = false;
    std::string listTag, listPriority;
    todoList->add_flag("--today", listToday, "오늘 마감만");
    todoList->add_flag("--overdue", listOverdue, "기한 초과 미완료만");
    todoList->add_option("--tag", listTag, "태그 필터");
    todoList->add_option("--priority", listPriority, "우선순위 필터 high|medium|low");

    auto* todoDone = todo->add_subcommand("done", "완료 처리");
    std::string doneId;
    todoDone->add_option("--id", doneId, "Todo ID (uuid)")->required();

    CLI11_PARSE(app, argc, argv);

    try {
        // --- Composition Root ---
        planning::adapter_config::TomlConfigLoader config(configPath);
        planning::adapter_logger::SpdlogLogger logger(config.logConfig());

        SQLite::Database db(config.dbPath().string(),
                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db.exec("PRAGMA journal_mode = WAL");
        db.exec("PRAGMA foreign_keys = ON");
        planning::adapter_sqlite::MigrationRunner(db).run(TERRARIUM_MIGRATIONS_DIR);

        planning::adapter_sqlite::SqliteEventRepository eventRepo(db);
        planning::adapter_sqlite::SqliteTodoRepository todoRepo(db);
        planning::domain::ConflictDetector detector;
        planning::domain::StdUuidGenerator idGen;
        planning::adapter_cli::CliConflictPrompter prompter(std::cin, std::cout);

        pa::CreateEventUseCase createEvent(eventRepo, detector, idGen, prompter,
                                           logger);
        pa::ListEventsUseCase listEvents(eventRepo, logger);
        pa::UpdateEventUseCase updateEvent(eventRepo, detector, prompter, logger);
        pa::DeleteEventUseCase deleteEvent(eventRepo, logger);
        pa::AddTodoUseCase addTodo(todoRepo, idGen, logger);
        pa::ListTodosUseCase listTodos(todoRepo, logger);
        pa::MarkTodoDoneUseCase markTodoDone(todoRepo, logger);
        pa::ShowDashboardUseCase dashboard(eventRepo, todoRepo, logger);

        if (eventAdd->parsed()) {
            pa::CreateEventCommand cmd;
            cmd.title = title;
            cmd.start = parseDateTime(startStr);
            if (!endStr.empty()) cmd.end = parseDateTime(endStr);
            cmd.allDay = false;
            const auto result = createEvent.execute(cmd);
            if (result.cancelledByUser) {
                std::cout << "취소되었습니다.\n";
            } else {
                std::cout << "Event '" << title << "' 추가 완료\n";
            }
        } else if (eventList->parsed()) {
            const auto today = localTodayDate();
            pa::ListEventsQuery q;
            if (!evFrom.empty() || !evTo.empty()) {
                const auto from = evFrom.empty() ? today : parseDate(evFrom);
                const auto to = evTo.empty() ? from : parseDate(evTo);
                q.rangeStart = localMidnightUtc(from);
                q.rangeEnd = localMidnightUtc(to + std::chrono::days{1});  // to 포함
            } else if (evWeek) {
                q.rangeStart = localMidnightUtc(today);
                q.rangeEnd = localMidnightUtc(today + std::chrono::days{7});
            } else {
                q.rangeStart = localMidnightUtc(today);
                q.rangeEnd = localMidnightUtc(today + std::chrono::days{1});
            }

            const auto events = listEvents.execute(q);
            if (events.empty()) {
                std::cout << "(이벤트 없음)\n";
            }
            for (const auto& e : events) {
                std::cout << uuids::to_string(e.id()) << "  "
                          << formatDateTime(e.timeRange().start());
                if (e.timeRange().end()) {
                    std::cout << " ~ " << formatDateTime(*e.timeRange().end());
                }
                std::cout << "  " << e.title();
                if (e.recurrenceRule()) std::cout << "  (반복)";
                std::cout << "\n";
            }
        } else if (eventUpdate->parsed()) {
            const auto parsed = uuids::uuid::from_string(evUpId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + evUpId);
            pa::UpdateEventCommand cmd;
            cmd.id = *parsed;
            if (oUpTitle->count()) cmd.title = evUpTitle;
            if (oUpStart->count()) cmd.start = parseDateTime(evUpStart);
            if (oUpEnd->count()) cmd.end = parseDateTime(evUpEnd);
            const auto result = updateEvent.execute(cmd);
            if (result.cancelledByUser) {
                std::cout << "취소되었습니다.\n";
            } else {
                std::cout << "이벤트 수정 완료: " << evUpId << "\n";
            }
        } else if (eventDelete->parsed()) {
            const auto parsed = uuids::uuid::from_string(evDelId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + evDelId);
            deleteEvent.execute(pa::DeleteEventCommand{*parsed});
            std::cout << "이벤트 삭제됨: " << evDelId << "\n";
        } else if (event->parsed()) {
            std::cout << event->help();
        } else if (todoAdd->parsed()) {
            pa::AddTodoCommand cmd;
            cmd.title = todoTitle;
            cmd.priority = parsePriority(todoPriority);
            cmd.tags = todoTags;
            if (!todoDue.empty()) cmd.due = parseDate(todoDue);
            addTodo.execute(cmd);
            std::cout << "Todo '" << todoTitle << "' 추가 완료\n";
        } else if (todoList->parsed()) {
            pa::ListTodosQuery q;
            if (listToday) q.dueOn = localTodayDate();
            if (listOverdue) q.overdueAsOf = localTodayDate();
            if (!listTag.empty()) q.tag = listTag;
            if (!listPriority.empty()) q.priority = parsePriority(listPriority);

            const auto todos = listTodos.execute(q);
            if (todos.empty()) {
                std::cout << "(할 일 없음)\n";
            }
            for (const auto& t : todos) {
                std::cout << uuids::to_string(t.id()) << "  "
                          << (t.isDone() ? "[x] " : "[ ] ") << t.title() << "  ("
                          << priorityText(t.priority()) << ")";
                if (t.dueDate()) std::cout << "  due " << formatDate(*t.dueDate());
                if (!t.tags().empty()) {
                    std::cout << "  #";
                    for (std::size_t i = 0; i < t.tags().size(); ++i) {
                        std::cout << (i ? "," : "") << t.tags()[i];
                    }
                }
                std::cout << "\n";
            }
        } else if (todoDone->parsed()) {
            const auto parsed = uuids::uuid::from_string(doneId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + doneId);
            markTodoDone.execute(pa::MarkTodoDoneCommand{*parsed});
            std::cout << "완료 처리됨: " << doneId << "\n";
        } else if (todo->parsed()) {
            std::cout << todo->help();
        } else {
            const auto today = localTodayDate();
            pa::ShowDashboardQuery q;
            q.todayDate = today;  // 로컬 달력 날짜 (todo overdue 비교용)
            // 로컬 [오늘 자정, 내일 자정) 을 UTC instant 로 (event 범위용).
            q.dayStart = localMidnightUtc(today);
            q.dayEnd = localMidnightUtc(today + std::chrono::days{1});

            const auto r = dashboard.execute(q);
            std::cout << "오늘 일정 " << r.todayEventsCount
                      << "개 / 마감 임박 Todo " << r.overdueTodosCount << "개\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
