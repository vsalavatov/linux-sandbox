#ifndef SANDBOX_CGROUP_HANDLER_H
#define SANDBOX_CGROUP_HANDLER_H

#include <cstddef>
#include <libcgroup.h>

namespace sandbox
{
    
class CGroupHandler {
public:
    CGroupHandler(const char *name);
    ~CGroupHandler();

    void limitMemory(std::size_t bytes);
    void create();
    void attach();

private:
    cgroup* cg_;
    cgroup_controller* memory_;
    cgroup_controller* freezer_;
};

} // namespace sandbox


#endif
