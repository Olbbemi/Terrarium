#include "seed/StdUuidGenerator.hpp"

namespace planning::domain {

StdUuidGenerator::StdUuidGenerator()
    : engine_(std::random_device{}()), gen_(engine_) {}

uuids::uuid StdUuidGenerator::next() { return gen_(); }

}  // namespace planning::domain
