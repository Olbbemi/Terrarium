#pragma once

#include <random>

#include "trunk/domain/IdGenerator.hpp"
#include "uuid.h"

namespace planning::domain {

// stduuid 기반 무작위 UUID 생성기(IdGenerator 구현).
class StdUuidGenerator : public IdGenerator {
public:
    StdUuidGenerator();
    uuids::uuid next() override;

private:
    std::mt19937 engine_;
    uuids::uuid_random_generator gen_;
};

}  // namespace planning::domain
