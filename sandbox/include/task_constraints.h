#ifndef SANDBOX_RESOURCE_LIMITS_H
#define SANDBOX_RESOURCE_LIMITS_H

#include <cstddef>
#include <optional>
#include <filesystem>
#include <vector>

namespace sandbox
{

class TaskConstraints {
public:
    struct FileMapping {
        std::filesystem::path from;
        std::filesystem::path to;
    };

    TaskConstraints(
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
    );

    const std::optional<double> maxRealTimeSeconds;
    const std::optional<std::size_t> maxMemoryBytes;
    const std::size_t stackSize;
    const std::optional<std::size_t> maxForks;
    const std::optional<int> niceness;

    const bool newNetwork;
    const bool freezable;
    const bool preserveCapabilities;

    const std::optional<std::filesystem::path> fsImage;
    const std::filesystem::path workDir;
    const std::vector<FileMapping> fileMapping;

    const uid_t uid;
    const gid_t gid;
    
    // TODO signals ? 
    // TODO other things
};

} // namespace sandbox


#endif
