#pragma once

#include <chrono>
#include <string>

#include "uuid.h"

namespace planning::domain {

// 목표 (Aggregate Root). 목표값 대비 누적값으로 달성률을 표현한다.
class Goal {
public:
    using Id = uuids::uuid;

    // targetValue <= 0 이면 std::invalid_argument.
    // periodStart > periodEnd 이면 std::invalid_argument.
    Goal(Id id, std::string name, int targetValue, std::string unit,
         std::chrono::sys_days periodStart, std::chrono::sys_days periodEnd);

    Id id() const;
    const std::string& name() const;
    int targetValue() const;
    int currentValue() const;
    const std::string& unit() const;
    std::chrono::sys_days periodStart() const;
    std::chrono::sys_days periodEnd() const;

    void incrementCounter();  // 누적값 +1
    void rename(std::string newName);
    void updateTarget(int newTarget);  // <= 0 이면 std::invalid_argument
    void updatePeriod(std::chrono::sys_days start,
                      std::chrono::sys_days end);  // start > end 이면 invalid_argument

    // 달성률 = currentValue / targetValue. 상한 없음(초과 달성 시 1.0 초과 가능).
    double progressRatio() const;

private:
    Id id_;
    std::string name_;
    int targetValue_;
    int currentValue_;
    std::string unit_;
    std::chrono::sys_days periodStart_;
    std::chrono::sys_days periodEnd_;
};

}  // namespace planning::domain
