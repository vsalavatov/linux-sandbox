#include "task_constraints.h"

namespace sandbox
{

TaskConstraints::TaskConstraints(
    std::optional<double> maxRealTimeSeconds,
    std::optional<std::size_t> maxMemoryBytes, 
    std::optional<std::size_t> maxForks,
    bool newNetwork,
    bool freezable
) : maxRealTimeSeconds{maxRealTimeSeconds}
  , maxMemoryBytes{maxMemoryBytes}
  , maxForks{maxForks}
  , newNetwork{newNetwork}
  , freezable{freezable}
{}

} // namespace sandbox
