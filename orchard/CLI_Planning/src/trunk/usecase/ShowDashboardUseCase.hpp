#pragma once

#include "trunk/usecase/commands/DashboardCommands.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"
#include "trunk/ports/TodoRepository.hpp"

namespace planning::application {

// 진입 화면 요약: 오늘 일정 수 + 마감 지난 미완료 Todo 수.
class ShowDashboardUseCase {
public:
    ShowDashboardUseCase(ports::EventRepository& events,
                         ports::TodoRepository& todos, toolshed::log::Logger& logger);

    struct Result {
        int todayEventsCount;
        int overdueTodosCount;
    };

    Result execute(const ShowDashboardQuery& query);

private:
    ports::EventRepository& events_;
    ports::TodoRepository& todos_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
