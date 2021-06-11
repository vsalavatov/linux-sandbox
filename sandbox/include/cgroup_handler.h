#ifndef SANDBOX_CGROUP_HANDLER_H
#define SANDBOX_CGROUP_HANDLER_H

#include <cstddef>
#include <libcgroup.h>

namespace sandbox
{

void setLibCGroupLoggerLevel(int level = -1);

class CGroupHandler {
public:
    CGroupHandler(const char *name);
    ~CGroupHandler();

    void limitMemory(std::size_t bytes);
    void limitProcesses(std::size_t maxProcesses);

    void freeze();
    void thaw();

    void create();
    void attach();

    void loadFromKernel();
    void propagateToKernel();

private:
    cgroup_controller* obtainFreezerController_();

    cgroup* cg_;
    cgroup_controller* memory_;
    cgroup_controller* pids_;
    cgroup_controller* freezer_;
};

} // namespace sandbox


#endif
