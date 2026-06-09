#include "trunk/usecase/UpdateEventUseCase.hpp"

#include <stdexcept>
#include <vector>

#include "trunk/domain/Event.hpp"
#include "trunk/domain/TimeRange.hpp"

namespace planning::application {

UpdateEventUseCase::UpdateEventUseCase(ports::EventRepository& events,
                                       const domain::ConflictDetector& detector,
                                       ports::ConflictPrompter& prompter,
                                       toolshed::log::Logger& logger)
    : events_(events),
      detector_(detector),
      prompter_(prompter),
      logger_(logger) {}

UpdateEventUseCase::Result UpdateEventUseCase::execute(
    const UpdateEventCommand& cmd) {
    auto found = events_.findById(cmd.id);
    if (!found) {
        throw std::out_of_range("UpdateEventUseCase: event not found");
    }
    domain::Event event = *found;

    const bool timeChanged =
        cmd.start.has_value() || cmd.end.has_value() || cmd.allDay.has_value();

    // 병합된 시간 구간 계산.
    const auto start = cmd.start.value_or(event.timeRange().start());
    const auto end = cmd.end.has_value() ? *cmd.end : event.timeRange().end();
    const bool allDay = cmd.allDay.value_or(event.timeRange().isAllDay());
    domain::TimeRange newRange(start, end, allDay);

    if (timeChanged) {
        // 자기 자신은 충돌 대상에서 제외.
        std::vector<domain::Event> others;
        for (const auto& e : events_.findOverlapping(newRange)) {
            if (e.id() != cmd.id) others.push_back(e);
        }
        if (const auto conflict = detector_.detect(newRange, others)) {
            if (prompter_.promptOnConflict(*conflict) ==
                ports::ConflictPrompter::Choice::CANCEL) {
                logger_.audit("event.update", "cancelled by user due to conflict");
                return Result{true};
            }
        }
    }

    if (cmd.title) event.rename(*cmd.title);
    if (timeChanged) event.reschedule(newRange);
    if (cmd.recurrence) event.setRecurrence(*cmd.recurrence);

    events_.update(event);
    logger_.audit("event.update", "updated");
    return Result{false};
}

}  // namespace planning::application
