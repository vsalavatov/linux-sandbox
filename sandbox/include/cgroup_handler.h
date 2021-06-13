#ifndef SANDBOX_CGROUP_HANDLER_H
#define SANDBOX_CGROUP_HANDLER_H

#include <cstddef>
#include <libcgroup.h>

namespace sandbox
{

void setLibCGroupLoggerLevel(int level = -1);

class CGroupHandler {
public:
    CGroupHandler(const char *name, bool owning = true);
    ~CGroupHandler();

    static void libinit();
    static void setLibCGroupLoggerLevel(int level);

    void limitMemory(std::size_t bytes);
    void limitProcesses(std::size_t maxProcesses);

    void addFreezerController();
    void freeze();
    void thaw();

    void create();
    void attach();
    void attachTask(pid_t pid);

    void loadFromKernel();
    void propagateToKernel();

    void disown();

private:
    cgroup_controller* getController_(const char* name);

    cgroup* cg_;
    bool owning_;
};

} // namespace sandbox


#endif
