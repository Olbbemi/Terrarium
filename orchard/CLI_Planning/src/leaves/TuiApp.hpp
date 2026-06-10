#pragma once

#include "toolshed/log/Logger.hpp"
#include "trunk/usecase/AddTodoUseCase.hpp"
#include "trunk/usecase/CreateEventUseCase.hpp"
#include "trunk/usecase/CreateGoalUseCase.hpp"
#include "trunk/usecase/DeleteEventUseCase.hpp"
#include "trunk/usecase/DeleteGoalUseCase.hpp"
#include "trunk/usecase/DeleteTodoUseCase.hpp"
#include "trunk/usecase/ListEventsUseCase.hpp"
#include "trunk/usecase/ListGoalsUseCase.hpp"
#include "trunk/usecase/ListTodosUseCase.hpp"
#include "trunk/usecase/LogGoalUseCase.hpp"
#include "trunk/usecase/MarkTodoDoneUseCase.hpp"
#include "trunk/usecase/ShowDashboardUseCase.hpp"
#include "trunk/usecase/ShowGoalUseCase.hpp"
#include "trunk/usecase/UpdateEventUseCase.hpp"
#include "trunk/usecase/UpdateGoalUseCase.hpp"
#include "trunk/usecase/UpdateTodoUseCase.hpp"

namespace planning::ui {

// glass 가 조립한 유스케이스 참조 묶음. 16개 개별 인자 대신 한 번에 주입(handoff 4-5).
// TuiApp 이 패널 액션에서 호출. 코어는 불변, leaves/ui 만 신규 driving 어댑터.
struct UseCases {
    application::CreateEventUseCase&   createEvent;
    application::ListEventsUseCase&    listEvents;
    application::UpdateEventUseCase&   updateEvent;
    application::DeleteEventUseCase&   deleteEvent;
    application::AddTodoUseCase&       addTodo;
    application::ListTodosUseCase&     listTodos;
    application::MarkTodoDoneUseCase&  markTodoDone;
    application::UpdateTodoUseCase&    updateTodo;
    application::DeleteTodoUseCase&    deleteTodo;
    application::CreateGoalUseCase&    createGoal;
    application::LogGoalUseCase&       logGoal;
    application::ShowGoalUseCase&      showGoal;
    application::ListGoalsUseCase&     listGoals;
    application::UpdateGoalUseCase&    updateGoal;
    application::DeleteGoalUseCase&    deleteGoal;
    application::ShowDashboardUseCase& dashboard;
};

// 상주 FTXUI 대시보드. run() 이 전체화면 루프를 돌리고 종료코드를 돌려준다(q 또는 fatal).
// B1: 최소 골격(플레이스홀더 + q 종료). 패널 뷰는 B2, 입력 모달/쓰기는 B3.
class TuiApp {
public:
    TuiApp(UseCases usecases, toolshed::log::Logger& logger);
    int run();

private:
    UseCases uc_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::ui
