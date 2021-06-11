#include "task_constraints.h"

namespace sandbox
{

TaskConstraints::TaskConstraints(
    std::optional<double> maxRealTimeSeconds,
    std::optional<std::size_t> maxMemoryBytes, 
    std::optional<std::size_t> maxForks,
    std::optional<int> niceness,
    bool newNetwork
) : maxRealTimeSeconds{maxRealTimeSeconds}
  , maxMemoryBytes{maxMemoryBytes}
  , maxForks{maxForks}
  , newNetwork{newNetwork}
  , niceness{niceness}
{}

} // namespace sandbox
