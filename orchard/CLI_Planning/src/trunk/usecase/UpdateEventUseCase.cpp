#include "trunk/usecase/UpdateEventUseCase.hpp"

#include <stdexcept>
#include <vector>

#include "trunk/domain/Event.hpp"
#include "trunk/domain/TimeRange.hpp"

namespace planning::application {

UpdateEventUseCase::UpdateEventUseCase(ports::EventRepository& events,
                                       const domain::ConflictDetector& detector,
                                       toolshed::log::Logger& logger)
    : events_(events),
      detector_(detector),
      logger_(logger) {}

UpdateEventResult UpdateEventUseCase::execute(UpdateEventCommand cmd,
                                              bool force) {
    auto found = events_.findById(cmd.id);
    if (!found) {
        throw std::out_of_range("UpdateEventUseCase: event not found");
    }
    domain::Event event = *found;

    const bool timeChanged =
        cmd.start.has_value() || cmd.end.has_value() || cmd.allDay.has_value();

    // 병합된 시간 구간 계산. 잘못된 구간은 TimeRange 생성자가 거른다.
    const auto start = cmd.start.value_or(event.timeRange().start());
    const auto end = cmd.end.has_value() ? *cmd.end : event.timeRange().end();
    const bool allDay = cmd.allDay.value_or(event.timeRange().isAllDay());
    domain::TimeRange newRange(start, end, allDay);

    if (timeChanged && !force) {
        // 자기 자신은 충돌 대상에서 제외.
        std::vector<domain::Event> others;
        for (const auto& e : events_.findOverlapping(newRange)) {
            if (e.id() != cmd.id) others.push_back(e);
        }
        if (const auto conflict = detector_.detect(newRange, others)) {
            // 충돌은 예외가 아닌 정상 분기. 미저장·미기록으로 반환.
            return UpdateEventResult{*conflict, false};
        }
    }

    if (cmd.title) event.rename(*cmd.title);
    if (timeChanged) event.reschedule(newRange);
    if (cmd.recurrence) event.setRecurrence(*cmd.recurrence);

    events_.update(event);
    logger_.audit("event.update", force ? "updated (forced)" : "updated");
    return UpdateEventResult{std::nullopt, true};
}

}  // namespace planning::application
