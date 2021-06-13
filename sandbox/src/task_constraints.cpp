#include "task_constraints.h"

namespace sandbox
{

TaskConstraints::TaskConstraints(
    std::optional<double> maxRealTimeSeconds,
    std::optional<std::size_t> maxMemoryBytes, 
    std::size_t stackSize,
    std::optional<std::size_t> maxForks,
    std::optional<int> niceness,
    bool newNetwork,
    bool freezable,
    bool preserveCapabilities,
    std::optional<std::filesystem::path> fsImage,
    std::filesystem::path workDir,
    std::vector<FileMapping> fileMapping,
    uid_t uid,
    gid_t gid
) : maxRealTimeSeconds{maxRealTimeSeconds}
  , maxMemoryBytes{maxMemoryBytes}
  , stackSize{stackSize}
  , maxForks{maxForks}
  , niceness{niceness}
  , newNetwork{newNetwork}
  , freezable{freezable}
  , preserveCapabilities{preserveCapabilities}
  , fsImage{fsImage}
  , workDir{workDir}
  , fileMapping{std::move(fileMapping)}
  , uid{uid}
  , gid{gid}
{}

} // namespace sandbox
