#include "leaves/TuiApp.hpp"

#include <chrono>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "leaves/CliFormat.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/Goal.hpp"
#include "trunk/domain/Todo.hpp"
#include "trunk/usecase/commands/DashboardCommands.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/usecase/commands/TodoCommands.hpp"

namespace planning::ui {

namespace {

// 패널 인덱스(handoff 5-5). 0=Today(읽기전용) / 1=Events / 2=Todos / 3=Goals.
enum Panel { kToday = 0, kEvents = 1, kTodos = 2, kGoals = 3 };

}  // namespace

TuiApp::TuiApp(UseCases usecases, toolshed::log::Logger& logger)
    : uc_(usecases), logger_(logger) {}

int TuiApp::run() {
    using namespace ftxui;
    namespace ch = std::chrono;

    auto screen = ScreenInteractive::Fullscreen();

    // --- view-state: 패널별 표시 줄 + Today 요약 (모든 변경 후 재조회로 재구성, 5-1) ---
    std::vector<std::string> eventLines, todoLines, goalLines;
    std::string todaySummary;

    // 조회 유스케이스 재호출 -> view-state 재구성. 벡터는 in-place 갱신(주소 안정).
    auto reload = [&] {
        const auto today = localToday();

        eventLines.clear();
        application::ListEventsQuery eq;
        eq.rangeStart = localMidnightUtc(today);
        eq.rangeEnd = localMidnightUtc(today + ch::days{1});
        for (const auto& e : uc_.listEvents.execute(eq)) {
            std::string line = formatDateTime(e.timeRange().start());
            if (e.timeRange().end()) {
                line += " -> " + formatDateTime(*e.timeRange().end());
            }
            line += "  " + e.title();
            if (e.recurrenceRule()) {
                line += std::string("  (") +
                        frequencyText(e.recurrenceRule()->frequency) + ")";
            }
            eventLines.push_back(line);
        }

        todoLines.clear();
        application::ListTodosQuery tq;  // 필터 없음 = 전체(작업 패널)
        for (const auto& t : uc_.listTodos.execute(tq)) {
            std::string line = (t.isDone() ? "[x] " : "[ ] ") + t.title();
            line += std::string("  (") + priorityText(t.priority()) + ")";
            if (t.dueDate()) line += "  due " + formatDate(*t.dueDate());
            if (!t.tags().empty()) {
                line += "  #";
                for (std::size_t i = 0; i < t.tags().size(); ++i) {
                    line += (i ? "," : "") + t.tags()[i];
                }
            }
            todoLines.push_back(line);
        }

        goalLines.clear();
        for (const auto& g : uc_.listGoals.execute()) {
            const int pct = static_cast<int>(g.progressRatio() * 100 + 0.5);
            std::string line = g.name() + "  " + progressBar(g.progressRatio()) +
                               " " + std::to_string(pct) + "%  (" +
                               std::to_string(g.currentValue()) + "/" +
                               std::to_string(g.targetValue()) + " " + g.unit() +
                               ")";
            goalLines.push_back(line);
        }

        application::ShowDashboardQuery dq;
        dq.todayDate = today;
        dq.dayStart = localMidnightUtc(today);
        dq.dayEnd = localMidnightUtc(today + ch::days{1});
        const auto r = uc_.dashboard.execute(dq);
        todaySummary = "오늘 일정 " + std::to_string(r.todayEventsCount) +
                       "개   /   기한 초과 Todo " +
                       std::to_string(r.overdueTodosCount) + "개";
    };
    reload();

    // --- 컴포넌트: 3 리스트 패널 = Menu(선택/스크롤 내장) + Today(읽기전용 inert) ---
    int selEvent = 0, selTodo = 0, selGoal = 0;
    auto eventsMenu = Menu(&eventLines, &selEvent);
    auto todosMenu = Menu(&todoLines, &selTodo);
    auto goalsMenu = Menu(&goalLines, &selGoal);
    auto todayPanel = Renderer([] { return text(""); });  // 포커스 가능하나 inert

    // 단일 포커스 상태: activePanel(0..3) = Container 선택 인덱스.
    // 좌우 화살표는 Container 가 패널 전환, 상하 화살표는 활성 Menu 가 소비.
    int activePanel = kEvents;
    auto container = Container::Horizontal(
        {todayPanel, eventsMenu, todosMenu, goalsMenu}, &activePanel);

    // 패널 박스: 제목(활성 시 강조) + 본문. 활성 패널은 테두리/제목을 초록·반전.
    auto panelBox = [&](int idx, const std::string& title,
                        Element body) -> Element {
        const bool active = (activePanel == idx);
        Element head = text(title) | bold | (active ? inverted : dim);
        Element box = vbox({head, separator(), body | flex}) | border;
        return active ? (box | color(Color::Green)) : box;
    };

    // 리스트 본문: 비었으면 placeholder, 아니면 Menu 렌더(+스크롤 인디케이터/frame).
    auto listBody = [&](Component menu, const std::vector<std::string>& lines,
                        const std::string& empty) -> Element {
        if (lines.empty()) return text(empty) | dim | center;
        return menu->Render() | vscroll_indicator | frame;
    };

    auto view = Renderer(container, [&] {
        Element today = todaySummary.empty() ? (text("(요약 없음)") | dim)
                                             : (text(todaySummary) | center);
        return vbox({
                   text("Terrarium 대시보드") | bold | hcenter,
                   panelBox(kToday, "1 Today", today) |
                       size(HEIGHT, EQUAL, 4),
                   hbox({
                       panelBox(kEvents, "2 Events",
                                listBody(eventsMenu, eventLines,
                                         "(이벤트 없음)")) |
                           flex,
                       panelBox(kTodos, "3 Todos",
                                listBody(todosMenu, todoLines, "(할 일 없음)")) |
                           flex,
                       panelBox(kGoals, "4 Goals",
                                listBody(goalsMenu, goalLines, "(목표 없음)")) |
                           flex,
                   }) | flex,
                   text("1-4: 패널  up/down: 이동/스크롤  left/right: 패널 전환  "
                        "q: 종료") |
                       dim | hcenter,
               }) |
               border;
    });

    // 종료 = q. 숫자 1-4 로 패널 직접 전환(Container 선택 인덱스 변경).
    // 그 외(화살표 등)는 false 반환해 Container/Menu 가 처리.
    auto root = CatchEvent(view, [&](const Event& event) {
        if (event == Event::Character('q')) {
            screen.Exit();
            return true;
        }
        for (int i = 0; i < 4; ++i) {
            if (event == Event::Character(static_cast<char>('1' + i))) {
                activePanel = i;
                return true;
            }
        }
        return false;
    });

    logger_.info("tui.start");
    screen.Loop(root);
    logger_.audit("tui.session", "ended");
    return 0;
}

}  // namespace planning::ui
