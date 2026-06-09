#include "trunk/usecase/ShowDashboardUseCase.hpp"

#include <chrono>

namespace planning::application {

ShowDashboardUseCase::ShowDashboardUseCase(ports::EventRepository& events,
                                           ports::TodoRepository& todos,
                                           toolshed::log::Logger& logger)
    : events_(events), todos_(todos), logger_(logger) {}

ShowDashboardUseCase::Result ShowDashboardUseCase::execute(
    const ShowDashboardQuery& query) {
    // NOTE: findInRange 는 비반복 기준. 반복 인스턴스 카운트는 추후 보완(TODO).
    const auto todayEvents = events_.findInRange(query.dayStart, query.dayEnd);
    const auto overdue = todos_.findOverdue(query.todayDate);

    logger_.audit("dashboard.show", "viewed");
    return Result{static_cast<int>(todayEvents.size()),
                  static_cast<int>(overdue.size())};
}

}  // namespace planning::application
