#include "trunk/usecase/ListTodosUseCase.hpp"

#include <algorithm>
#include <string>

namespace planning::application {

ListTodosUseCase::ListTodosUseCase(ports::TodoRepository& todos,
                                   toolshed::log::Logger& logger)
    : todos_(todos), logger_(logger) {}

std::vector<domain::Todo> ListTodosUseCase::execute(const ListTodosQuery& query) {
    std::vector<domain::Todo> out;
    // NOTE(NF1): 현재는 findAll 후 인메모리 필터. 어댑터 단계에서 인덱스 조회로 최적화.
    for (const auto& t : todos_.findAll()) {
        if (query.dueOn &&
            !(t.dueDate() && *t.dueDate() == *query.dueOn)) {
            continue;
        }
        if (query.overdueAsOf &&
            !(!t.isDone() && t.dueDate() && *t.dueDate() < *query.overdueAsOf)) {
            continue;
        }
        if (query.tag) {
            const auto& tags = t.tags();
            if (std::find(tags.begin(), tags.end(), *query.tag) == tags.end()) {
                continue;
            }
        }
        if (query.priority && t.priority() != *query.priority) {
            continue;
        }
        out.push_back(t);
    }

    logger_.audit("todo.list", "listed");
    return out;
}

}  // namespace planning::application
