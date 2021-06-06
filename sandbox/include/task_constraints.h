#ifndef SANDBOX_RESOURCE_LIMITS_H
#define SANDBOX_RESOURCE_LIMITS_H

#include <cstddef>
#include <optional>

namespace sandbox
{

class TaskConstraints {
public:
    TaskConstraints(
        std::optional<double> maxRealTimeSeconds,
        std::optional<std::size_t> maxMemoryBytes, 
        std::optional<std::size_t> maxForks
    );

    const std::optional<double> maxRealTimeSeconds;
    const std::optional<std::size_t> maxMemoryBytes;
    const std::optional<std::size_t> maxForks;
    // TODO file access
    // TODO signals ? 
    // TODO other things
};

} // namespace sandbox


#endif
