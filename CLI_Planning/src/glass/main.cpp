#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include "climate/TomlConfigLoader.hpp"
#include "leaves/CliConflictPrompter.hpp"
#include "leaves/CliFormat.hpp"
#include "rings/SpdlogLogger.hpp"
#include "roots/MigrationRunner.hpp"
#include "roots/SqliteEventRepository.hpp"
#include "roots/SqliteGoalRepository.hpp"
#include "roots/SqliteTodoRepository.hpp"
#include "seed/ConflictDetector.hpp"
#include "seed/Goal.hpp"
#include "seed/Priority.hpp"
#include "seed/RecurrenceRule.hpp"
#include "seed/StdUuidGenerator.hpp"
#include "seed/Todo.hpp"
#include "stem/AddTodoUseCase.hpp"
#include "stem/CreateEventUseCase.hpp"
#include "stem/CreateGoalUseCase.hpp"
#include "stem/DeleteEventUseCase.hpp"
#include "stem/DeleteGoalUseCase.hpp"
#include "stem/ListEventsUseCase.hpp"
#include "stem/ListGoalsUseCase.hpp"
#include "stem/ListTodosUseCase.hpp"
#include "stem/LogGoalUseCase.hpp"
#include "stem/MarkTodoDoneUseCase.hpp"
#include "stem/ShowDashboardUseCase.hpp"
#include "stem/ShowGoalUseCase.hpp"
#include "stem/UpdateEventUseCase.hpp"
#include "stem/UpdateGoalUseCase.hpp"
#include "stem/commands/DashboardCommands.hpp"
#include "stem/commands/EventCommands.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "stem/commands/TodoCommands.hpp"

namespace {

namespace pa = planning::application;

// CLI 입출력 변환(파싱·포맷·로컬↔UTC·진행 막대)은 테스트 가능한 leaves 어댑터로 분리.
// 여기서는 그 이름들을 그대로 끌어와 사용한다.
using namespace planning::adapter_cli;

// 시스템 로컬 타임존 기준 오늘 달력 날짜(정책 A). 벽시계를 읽으므로 Composition Root 에 둔다.
std::chrono::sys_days localTodayDate() {
    const auto* zone = std::chrono::current_zone();
    const std::chrono::local_days ld = std::chrono::floor<std::chrono::days>(
        zone->to_local(std::chrono::system_clock::now()));
    // 로컬 civil 날짜의 day-count 를 그대로 sys_days(civil date)로 재해석.
    return std::chrono::sys_days{ld.time_since_epoch()};
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Terrarium (gaia) - CLI 일정 관리 (vertical slice)"};
    app.require_subcommand(0);  // 서브커맨드 없으면 대시보드

    std::string configPath;
    app.add_option("--config", configPath, "TOML 설정 파일 경로")->required();

    auto* event = app.add_subcommand("event", "이벤트 관리");
    auto* eventAdd = event->add_subcommand("add", "이벤트 추가");
    std::string title, startStr, endStr;
    std::string evRepeat, evUntil;
    eventAdd->add_option("--title", title, "제목")->required();
    eventAdd->add_option("--start", startStr, "시작 (YYYY-MM-DDTHH:MM)")->required();
    eventAdd->add_option("--end", endStr, "종료 (YYYY-MM-DDTHH:MM)");
    eventAdd->add_option("--repeat", evRepeat, "반복 daily|weekly|monthly|yearly");
    eventAdd->add_option("--until", evUntil, "반복 종료일 (YYYY-MM-DD, 포함)");

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
    bool evUpClearEnd = false;
    eventUpdate->add_flag("--clear-end", evUpClearEnd, "종료시각 제거");
    bool evUpAllDay = false, evUpNoAllDay = false;
    eventUpdate->add_flag("--all-day", evUpAllDay, "하루 종일로 표시");
    eventUpdate->add_flag("--no-all-day", evUpNoAllDay, "하루 종일 해제");
    std::string evUpRepeat, evUpUntil;
    bool evUpNoRepeat = false;
    auto* oUpRepeat = eventUpdate->add_option("--repeat", evUpRepeat,
                                              "반복 daily|weekly|monthly|yearly");
    eventUpdate->add_option("--until", evUpUntil, "반복 종료일 (YYYY-MM-DD, 포함)");
    eventUpdate->add_flag("--no-repeat", evUpNoRepeat, "반복 제거");

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

    auto* goal = app.add_subcommand("goal", "목표 관리");

    auto* goalCreate = goal->add_subcommand("create", "목표 생성");
    std::string gName, gUnit, gFrom, gTo;
    int gTarget = 0;
    goalCreate->add_option("--name", gName, "이름")->required();
    goalCreate->add_option("--target", gTarget, "목표값 (양의 정수)")->required();
    goalCreate->add_option("--unit", gUnit, "단위 (예: 회, km)")->required();
    goalCreate->add_option("--from", gFrom, "기간 시작 (YYYY-MM-DD)")->required();
    goalCreate->add_option("--to", gTo, "기간 종료 (YYYY-MM-DD)")->required();

    auto* goalLog = goal->add_subcommand("log", "진행 +1 (이름으로)");
    std::string gLogName;
    goalLog->add_option("--name", gLogName, "목표 이름")->required();

    auto* goalShow = goal->add_subcommand("show", "목표 진행 보기 (이름으로)");
    std::string gShowName;
    goalShow->add_option("--name", gShowName, "목표 이름")->required();

    auto* goalList = goal->add_subcommand("list", "목표 목록");

    auto* goalUpdate = goal->add_subcommand("update", "목표 수정 (부분)");
    std::string gUpId, gUpName, gUpFrom, gUpTo;
    int gUpTarget = 0;
    goalUpdate->add_option("--id", gUpId, "목표 ID (uuid)")->required();
    auto* oGName = goalUpdate->add_option("--name", gUpName, "이름");
    auto* oGTarget = goalUpdate->add_option("--target", gUpTarget, "목표값");
    auto* oGFrom = goalUpdate->add_option("--from", gUpFrom, "기간 시작 (YYYY-MM-DD)");
    auto* oGTo = goalUpdate->add_option("--to", gUpTo, "기간 종료 (YYYY-MM-DD)");

    auto* goalDelete = goal->add_subcommand("delete", "목표 삭제");
    std::string gDelId;
    goalDelete->add_option("--id", gDelId, "목표 ID (uuid)")->required();

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
        planning::adapter_sqlite::SqliteGoalRepository goalRepo(db);
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
        pa::CreateGoalUseCase createGoal(goalRepo, idGen, logger);
        pa::LogGoalUseCase logGoal(goalRepo, logger);
        pa::ShowGoalUseCase showGoal(goalRepo, logger);
        pa::ListGoalsUseCase listGoals(goalRepo, logger);
        pa::UpdateGoalUseCase updateGoal(goalRepo, logger);
        pa::DeleteGoalUseCase deleteGoal(goalRepo, logger);
        pa::ShowDashboardUseCase dashboard(eventRepo, todoRepo, logger);

        // --repeat/--until → RecurrenceRule. until 은 해당 로컬 날짜의 끝(23:59:59)으로
        // 잡아 그 날의 occurrence 까지 포함한다.
        auto makeRule = [](const std::string& freq, const std::string& until) {
            planning::domain::RecurrenceRule rule;
            rule.frequency = parseFrequency(freq);
            if (!until.empty()) {
                rule.until = localMidnightUtc(parseDate(until) + std::chrono::days{1}) -
                             std::chrono::seconds{1};
            }
            return rule;
        };

        if (eventAdd->parsed()) {
            pa::CreateEventCommand cmd;
            cmd.title = title;
            cmd.start = parseDateTime(startStr);
            if (!endStr.empty()) cmd.end = parseDateTime(endStr);
            cmd.allDay = false;
            if (!evRepeat.empty()) cmd.recurrence = makeRule(evRepeat, evUntil);
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
                    std::cout << " -> " << formatDateTime(*e.timeRange().end());
                }
                std::cout << "  " << e.title();
                if (e.recurrenceRule()) {
                    std::cout << "  (" << frequencyText(e.recurrenceRule()->frequency)
                              << ")";
                }
                std::cout << "\n";
            }
        } else if (eventUpdate->parsed()) {
            const auto parsed = uuids::uuid::from_string(evUpId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + evUpId);
            pa::UpdateEventCommand cmd;
            cmd.id = *parsed;
            if (oUpTitle->count()) cmd.title = evUpTitle;
            if (oUpStart->count()) cmd.start = parseDateTime(evUpStart);
            if (oUpEnd->count() && evUpClearEnd) {
                throw std::runtime_error("--end 와 --clear-end 는 함께 쓸 수 없습니다");
            }
            if (oUpEnd->count()) {
                cmd.end = std::optional<std::chrono::sys_seconds>{
                    parseDateTime(evUpEnd)};
            } else if (evUpClearEnd) {
                cmd.end = std::optional<std::chrono::sys_seconds>{std::nullopt};
            }
            if (evUpAllDay && evUpNoAllDay) {
                throw std::runtime_error(
                    "--all-day 와 --no-all-day 는 함께 쓸 수 없습니다");
            }
            if (evUpAllDay) cmd.allDay = true;
            else if (evUpNoAllDay) cmd.allDay = false;
            if (evUpNoRepeat && oUpRepeat->count()) {
                throw std::runtime_error("--repeat 와 --no-repeat 는 함께 쓸 수 없습니다");
            }
            if (evUpNoRepeat) {
                cmd.recurrence = std::optional<planning::domain::RecurrenceRule>{};
            } else if (oUpRepeat->count()) {
                cmd.recurrence = std::optional<planning::domain::RecurrenceRule>{
                    makeRule(evUpRepeat, evUpUntil)};
            }
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
        } else if (goalCreate->parsed()) {
            pa::CreateGoalCommand cmd;
            cmd.name = gName;
            cmd.targetValue = gTarget;
            cmd.unit = gUnit;
            cmd.periodStart = parseDate(gFrom);
            cmd.periodEnd = parseDate(gTo);
            createGoal.execute(cmd);
            std::cout << "Goal '" << gName << "' 생성 완료\n";
        } else if (goalLog->parsed()) {
            logGoal.execute(pa::LogGoalCommand{gLogName});
            std::cout << "기록됨: " << gLogName << " +1\n";
        } else if (goalShow->parsed()) {
            // show 유스케이스가 저장소 findByName 으로 직접 조회(전체 스캔 회피).
            const auto r = showGoal.execute(pa::ShowGoalQuery{gShowName});
            const int pct = static_cast<int>(r.progressRatio * 100 + 0.5);
            std::cout << r.name << "  " << progressBar(r.progressRatio) << " " << pct
                      << "%  (" << r.currentValue << "/" << r.targetValue << " "
                      << r.unit << ")\n";
        } else if (goalList->parsed()) {
            const auto goals = listGoals.execute();
            if (goals.empty()) {
                std::cout << "(목표 없음)\n";
            }
            for (const auto& g : goals) {
                const int pct = static_cast<int>(g.progressRatio() * 100 + 0.5);
                std::cout << uuids::to_string(g.id()) << "  " << g.name() << "  "
                          << progressBar(g.progressRatio()) << " " << pct << "%  ("
                          << g.currentValue() << "/" << g.targetValue() << " "
                          << g.unit() << ")  " << formatDate(g.periodStart()) << "~"
                          << formatDate(g.periodEnd()) << "\n";
            }
        } else if (goalUpdate->parsed()) {
            const auto parsed = uuids::uuid::from_string(gUpId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + gUpId);
            pa::UpdateGoalCommand cmd;
            cmd.id = *parsed;
            if (oGName->count()) cmd.name = gUpName;
            if (oGTarget->count()) cmd.targetValue = gUpTarget;
            if (oGFrom->count() || oGTo->count()) {
                if (!oGFrom->count() || !oGTo->count()) {
                    throw std::runtime_error("기간 변경은 --from 과 --to 를 함께 지정해야 합니다");
                }
                cmd.period = std::make_pair(parseDate(gUpFrom), parseDate(gUpTo));
            }
            updateGoal.execute(cmd);
            std::cout << "목표 수정 완료: " << gUpId << "\n";
        } else if (goalDelete->parsed()) {
            const auto parsed = uuids::uuid::from_string(gDelId);
            if (!parsed) throw std::runtime_error("잘못된 ID 형식: " + gDelId);
            deleteGoal.execute(pa::DeleteGoalCommand{*parsed});
            std::cout << "목표 삭제됨: " << gDelId << "\n";
        } else if (goal->parsed()) {
            std::cout << goal->help();
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
