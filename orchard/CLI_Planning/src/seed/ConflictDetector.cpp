#include "seed/ConflictDetector.hpp"

namespace planning::domain {

std::optional<Conflict> ConflictDetector::detect(
    const TimeRange& candidate,
    const std::vector<Event>& existingOverlapping) const {
    for (const Event& e : existingOverlapping) {
        if (e.timeRange().overlaps(candidate)) {
            return Conflict{e.id(), e.title(), e.timeRange()};
        }
    }
    return std::nullopt;
}

}  // namespace planning::domain
