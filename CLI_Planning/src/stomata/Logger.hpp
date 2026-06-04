#pragma once

#include <string>

namespace planning::ports {

class Logger {
public:
    virtual ~Logger() = default;

    virtual void debug(const std::string&) = 0;
    virtual void info(const std::string&) = 0;
    virtual void warn(const std::string&) = 0;
    virtual void error(const std::string&) = 0;
    virtual void audit(const std::string& action, const std::string& detail) = 0;
};

}  // namespace planning::ports
