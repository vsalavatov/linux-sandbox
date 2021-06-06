#ifndef SANDBOX_TASK_H
#define SANDBOX_TASK_H

#include <vector>
#include <string>
#include <filesystem>

#include "task_constraints.h"

namespace sandbox
{

class Task {
public:
    Task() = delete;
    Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints);
    virtual ~Task() = default;
private:
    std::filesystem::path executable_;
    std::vector<std::string> args_;
    TaskConstraints constraints_;
};

} // namespace sandbox

#endif
