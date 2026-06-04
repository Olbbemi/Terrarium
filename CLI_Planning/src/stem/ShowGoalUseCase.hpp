#pragma once

#include <string>

#include "stem/commands/GoalCommands.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

// 목표 진행 데이터를 반환. ASCII 막대 렌더링은 어댑터(leaves) 책임.
class ShowGoalUseCase {
public:
    ShowGoalUseCase(ports::GoalRepository& goals, ports::Logger& logger);

    struct Result {
        std::string name;
        int currentValue;
        int targetValue;
        std::string unit;
        double progressRatio;  // 상한 없음(초과 달성 시 1.0 초과 가능)
    };

    // 대상이 없으면 std::out_of_range.
    Result execute(const ShowGoalQuery& query);

private:
    ports::GoalRepository& goals_;
    ports::Logger& logger_;
};

}  // namespace planning::application
