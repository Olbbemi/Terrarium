#include "leaves/TuiApp.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
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

    // --- view-state: 패널별 도메인 객체 + 표시 줄 + Today 요약 (5-1) ---
    std::vector<domain::Event> loadedEvents;
    std::vector<std::string> eventLines, todoLines, goalLines;
    std::string todaySummary;
    int selEvent = 0, selTodo = 0, selGoal = 0;

    // "YYYY-MM-DD HH:MM" 표시형 -> "YYYY-MM-DDTHH:MM" 입력형(폼 prefill 라운드트립).
    auto toInputDateTime = [](ch::sys_seconds t) {
        std::string s = formatDateTime(t);
        if (s.size() > 10) s[10] = 'T';
        return s;
    };

    // 조회 유스케이스 재호출 -> view-state 재구성. 벡터는 in-place 갱신(주소 안정).
    auto reload = [&] {
        const auto today = localToday();

        loadedEvents.clear();
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
            loadedEvents.push_back(e);
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

        // 선택 인덱스 클램프(삭제 등으로 줄어든 경우).
        auto clamp = [](int& sel, std::size_t n) {
            if (n == 0) sel = 0;
            else if (sel >= static_cast<int>(n)) sel = static_cast<int>(n) - 1;
        };
        clamp(selEvent, eventLines.size());
        clamp(selTodo, todoLines.size());
        clamp(selGoal, goalLines.size());
    };
    reload();

    // --- B3a 쓰기 상태: 이벤트 모달 폼 + 충돌 모달 + 검증 배너/토스트 ---
    bool showEventForm = false;   // 추가/수정 폼 모달
    bool showConflict = false;    // 2-phase 충돌 모달
    bool editMode = false;        // false=추가, true=수정
    domain::Event::Id editEventId{};
    std::string fTitle, fStart, fEnd;  // 폼 필드(반복 입력은 B3a 보류)
    std::string formError;             // 모달 배너(검증 실패 -- 모달 유지)
    std::string conflictMsg;           // 충돌 모달 문구
    std::string toast;                 // 비모달 액션 결과(x 삭제 등)
    application::CreateEventCommand pendingCreate;  // 충돌 시 force 재시도용
    application::UpdateEventCommand pendingUpdate;
    bool pendingIsEdit = false;

    // 폼 필드 -> Command 빌드 -> 실행. 5-3: 성공만 모달 닫음, 실패는 배너로 유지.
    auto submitEvent = [&] {
        formError.clear();
        try {
            if (!editMode) {
                application::CreateEventCommand cmd;
                cmd.title = fTitle;                  // 도메인 불변식(빈 제목)은 throw
                cmd.start = parseDateTime(fStart);   // UI 파싱(형식 오류 throw)
                if (!fEnd.empty()) cmd.end = parseDateTime(fEnd);
                cmd.allDay = false;
                const auto r = uc_.createEvent.execute(cmd);
                if (r.conflict) {  // 2-phase: 충돌이면 모달로 되묻기
                    pendingCreate = cmd;
                    pendingIsEdit = false;
                    conflictMsg = "기존 '" + r.conflict->existingTitle +
                                  "' 와 시간이 겹칩니다. 그래도 추가할까요?";
                    showConflict = true;
                    return;
                }
                showEventForm = false;
                reload();
            } else {
                application::UpdateEventCommand cmd;
                cmd.id = editEventId;
                cmd.title = fTitle;
                if (!fStart.empty()) cmd.start = parseDateTime(fStart);
                if (!fEnd.empty()) {
                    cmd.end = std::optional<ch::sys_seconds>{parseDateTime(fEnd)};
                }
                const auto r = uc_.updateEvent.execute(cmd);
                if (r.conflict) {
                    pendingUpdate = cmd;
                    pendingIsEdit = true;
                    conflictMsg = "기존 '" + r.conflict->existingTitle +
                                  "' 와 시간이 겹칩니다. 그래도 저장할까요?";
                    showConflict = true;
                    return;
                }
                showEventForm = false;
                reload();
            }
        } catch (const std::exception& e) {
            formError = e.what();  // 모달 유지(UI 파싱 + 도메인 불변식 공통 배너)
        }
    };

    // 충돌 강행(force=true). 성공 시 두 모달 닫고 재조회.
    auto forceCommit = [&] {
        try {
            if (!pendingIsEdit) {
                uc_.createEvent.execute(pendingCreate, /*force=*/true);
            } else {
                uc_.updateEvent.execute(pendingUpdate, /*force=*/true);
            }
            showConflict = false;
            showEventForm = false;
            reload();
        } catch (const std::exception& e) {
            formError = e.what();
            showConflict = false;  // 폼으로 복귀해 배너 표시
        }
    };

    // --- 대시보드(읽기) 컴포넌트: B2 ---
    auto eventsMenu = Menu(&eventLines, &selEvent);
    auto todosMenu = Menu(&todoLines, &selTodo);
    auto goalsMenu = Menu(&goalLines, &selGoal);
    auto todayPanel = Renderer([] { return text(""); });

    // activePanel = 라우팅/하이라이트의 권위(1-4·좌우로 나만 갱신). focusIdx =
    // Container 키보드 포커스(스크롤용). 빈 메뉴는 Focusable=false 라 포커스 해소가
    // selector 를 드리프트시키므로, 라우팅 권위(activePanel)를 분리해 'a' 등이 항상
    // 동작하게 한다(설계 5-5: 빈 패널도 a 활성).
    int activePanel = kEvents;
    int focusIdx = kEvents;
    std::array<Component, 4> panels{todayPanel, eventsMenu, todosMenu, goalsMenu};
    auto container = Container::Horizontal(
        {todayPanel, eventsMenu, todosMenu, goalsMenu}, &focusIdx);

    auto panelBox = [&](int idx, const std::string& title,
                        Element body) -> Element {
        const bool active = (activePanel == idx);
        Element head = text(title) | bold | (active ? inverted : dim);
        Element box = vbox({head, separator(), body | flex}) | border;
        return active ? (box | color(Color::Green)) : box;
    };
    auto listBody = [&](Component menu, const std::vector<std::string>& lines,
                        const std::string& empty) -> Element {
        if (lines.empty()) return text(empty) | dim | center;
        return menu->Render() | vscroll_indicator | frame;
    };

    auto dashboard = Renderer(container, [&] {
        Element today = todaySummary.empty() ? (text("(요약 없음)") | dim)
                                             : (text(todaySummary) | center);
        Element footer =
            toast.empty()
                ? (text("1-4: 패널  up/down: 이동  a: 추가  e: 수정  x: 삭제  "
                        "q: 종료") |
                   dim)
                : (text(toast) | color(Color::Yellow));
        return vbox({
                   text("Terrarium 대시보드") | bold | hcenter,
                   panelBox(kToday, "1 Today", today) | size(HEIGHT, EQUAL, 4),
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
                   footer | hcenter,
               }) |
               border;
    });

    // --- 이벤트 폼 모달 컴포넌트 ---
    InputOption lineOpt;
    lineOpt.multiline = false;
    lineOpt.on_enter = [&] { submitEvent(); };
    auto inTitle = Input(&fTitle, lineOpt);
    auto inStart = Input(&fStart, lineOpt);
    auto inEnd = Input(&fEnd, lineOpt);
    auto btnSave = Button("저장", [&] { submitEvent(); }, ButtonOption::Ascii());
    auto btnCancel =
        Button("취소", [&] { showEventForm = false; }, ButtonOption::Ascii());
    auto formContainer = Container::Vertical({
        inTitle,
        inStart,
        inEnd,
        Container::Horizontal({btnSave, btnCancel}),
    });
    auto eventForm = Renderer(formContainer, [&] {
        auto field = [](const std::string& label, Component in) {
            return hbox({text(label) | size(WIDTH, EQUAL, 8),
                         in->Render() | flex | border});
        };
        Elements rows = {
            text(editMode ? "이벤트 수정" : "이벤트 추가") | bold,
            separator(),
            field("제목", inTitle),
            field("시작", inStart),
            field("종료", inEnd),
            text("형식: YYYY-MM-DDTHH:MM (종료는 비워둘 수 있음)") | dim,
        };
        if (!formError.empty()) {
            rows.push_back(text(formError) | color(Color::Red));
        }
        rows.push_back(separator());
        rows.push_back(hbox({btnSave->Render(), text("  "), btnCancel->Render()}));
        return vbox(rows) | border | size(WIDTH, GREATER_THAN, 54) |
               bgcolor(Color::Black);
    });

    // --- 충돌 모달 컴포넌트 ---
    auto btnForce =
        Button("강행", [&] { forceCommit(); }, ButtonOption::Ascii());
    auto btnConflictCancel =
        Button("취소", [&] { showConflict = false; }, ButtonOption::Ascii());
    auto conflictContainer =
        Container::Horizontal({btnForce, btnConflictCancel});
    auto conflictDialog = Renderer(conflictContainer, [&] {
        return vbox({
                   text("일정 충돌") | bold,
                   separator(),
                   paragraph(conflictMsg),
                   separator(),
                   hbox({btnForce->Render(), text("  "),
                         btnConflictCancel->Render()}),
               }) |
               border | size(WIDTH, GREATER_THAN, 44) | bgcolor(Color::Black);
    });

    // 모달 합성: 대시보드 위에 폼, 그 위에 충돌.
    auto withForm = Modal(dashboard, eventForm, &showEventForm);
    auto withConflict = Modal(withForm, conflictDialog, &showConflict);

    // --- 키 처리: 모달이 열려 있으면 모달/입력이 처리(여기선 통과) ---
    auto root = CatchEvent(withConflict, [&](const Event& event) {
        if (showEventForm || showConflict) return false;  // 모달 입력 우선

        if (event == Event::Character('q')) {
            screen.Exit();
            return true;
        }
        for (int i = 0; i < 4; ++i) {
            if (event == Event::Character(static_cast<char>('1' + i))) {
                activePanel = i;
                panels[i]->TakeFocus();
                return true;
            }
        }
        if (event == Event::ArrowLeft && activePanel > 0) {
            panels[--activePanel]->TakeFocus();
            return true;
        }
        if (event == Event::ArrowRight && activePanel < 3) {
            panels[++activePanel]->TakeFocus();
            return true;
        }

        // Events 패널 쓰기 액션(B3a). 다른 패널 a/e/d/x/l 은 B3b/B3c.
        if (activePanel == kEvents) {
            const bool hasSel =
                !loadedEvents.empty() &&
                selEvent < static_cast<int>(loadedEvents.size());
            if (event == Event::Character('a')) {
                editMode = false;
                fTitle.clear();
                fStart.clear();
                fEnd.clear();
                formError.clear();
                toast.clear();
                showEventForm = true;
                return true;
            }
            if (event == Event::Character('e')) {
                if (!hasSel) {
                    toast = "수정할 이벤트가 없습니다.";
                    return true;
                }
                const auto& ev = loadedEvents[selEvent];
                editMode = true;
                editEventId = ev.id();
                fTitle = ev.title();
                fStart = toInputDateTime(ev.timeRange().start());
                fEnd = ev.timeRange().end()
                           ? toInputDateTime(*ev.timeRange().end())
                           : std::string{};
                formError.clear();
                toast.clear();
                showEventForm = true;
                return true;
            }
            if (event == Event::Character('x')) {
                if (!hasSel) {
                    toast = "삭제할 이벤트가 없습니다.";
                    return true;
                }
                try {
                    uc_.deleteEvent.execute(
                        application::DeleteEventCommand{loadedEvents[selEvent].id()});
                    toast = "이벤트 삭제됨.";
                    reload();
                } catch (const std::exception& e) {
                    toast = std::string("삭제 실패: ") + e.what();
                }
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
