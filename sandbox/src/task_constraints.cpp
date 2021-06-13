#include "task_constraints.h"

namespace sandbox
{

TaskConstraints::TaskConstraints(
    std::optional<double> maxRealTimeSeconds,
    std::optional<std::size_t> maxMemoryBytes, 
    std::optional<std::size_t> maxForks,
    std::optional<int> niceness,
    bool newNetwork,
    bool freezable,
    std::optional<std::filesystem::path> fsImage,
    std::filesystem::path workDir,
    std::vector<FileMapping> fileMapping
) : maxRealTimeSeconds{maxRealTimeSeconds}
  , maxMemoryBytes{maxMemoryBytes}
  , maxForks{maxForks}
  , niceness{niceness}
  , newNetwork{newNetwork}
  , freezable{freezable}
  , fsImage{fsImage}
  , workDir{workDir}
  , fileMapping{std::move(fileMapping)}
{}

} // namespace sandbox
