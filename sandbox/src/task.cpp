#include "task.h"

namespace sandbox
{

Task::Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints)
    : executable_{std::move(executable)}
    , args_{std::move(args)}
    , constraints_{std::move(constraints)}
{}

} // namespace sandbox
