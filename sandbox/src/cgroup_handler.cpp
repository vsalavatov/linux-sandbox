#include "cgroup_handler.h"
#include "exceptions.h"
#include "msg.h"

#include <unistd.h>
#include <iostream>

namespace sandbox
{

static void libcgroup_logger_(void *userdata, int level, const char *fmt, va_list ap)
{
    auto m = impl::Message("(Sandbox) LIBCGROUP: ");
    vfprintf(stderr, fmt, ap);
}

CGroupHandler::CGroupHandler(const char *name, bool owning) : owning_{owning} {
    cg_ = cgroup_new_cgroup(name);
    if (!cg_) {
        throw SandboxError("failed to make new cgroup");
    }
}

CGroupHandler::~CGroupHandler() {
    if (owning_ && cg_) {
        if (int ret = cgroup_delete_cgroup(cg_, 0); ret) {
            impl::Message() << "Warning: failed to delete cgroup: " << cgroup_strerror(ret);
        }
        cgroup_free(&cg_);
    }
}

void CGroupHandler::libinit() {
    static bool inited = 0;
    if (inited) return;
    if (int ret = cgroup_init(); ret != 0) {
        impl::Message() << "failed to initialize libcgroup: " << cgroup_strerror(ret);
    }
    inited = true;
}

void CGroupHandler::setLibCGroupLoggerLevel(int level) {
    cgroup_set_logger(libcgroup_logger_, level, NULL);
}

void CGroupHandler::limitMemory(std::size_t bytes) {
    auto memory = cgroup_add_controller(cg_, "memory");
    if (!memory) {
        throw SandboxError("failed to initialize cgroup controller \"memory\"");
    }
    if (auto ret = cgroup_set_value_uint64(memory, "memory.max", bytes); ret) {
        throw SandboxError("failed to set memory limit: " + cgroup_strerror(ret));
    }
    if (auto ret = cgroup_set_value_uint64(memory, "memory.swap.max", 0); ret) { // disable swap
        throw SandboxError("failed to disable swap: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::limitProcesses(std::size_t maxProcesses) {
    auto pids = cgroup_add_controller(cg_, "pids");
    if (!pids) {
        throw SandboxError("failed to initialize cgroup controller \"pids\"");
    }
    if (auto ret = cgroup_set_value_uint64(pids, "pids.max", maxProcesses); ret) {
        throw SandboxError("failed to set limit of number of processes: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::addFreezerController() {
    if (!cgroup_add_controller(cg_, "freezer")) {
        throw SandboxError("failed to initialize cgroup controller \"freezer\"");
    }
}

void CGroupHandler::freeze() {
    auto freezeController = getController_("freezer");
    if (!freezeController) {
        throw SandboxError("failed to freeze: freezer controller is not available");
    }
    if (auto ret = cgroup_set_value_string(freezeController, "freezer.state", "FROZEN"); ret) {
        throw SandboxError("failed to freeze: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::thaw() {
    auto freezeController = getController_("freezer");
    if (!freezeController) {
        throw SandboxError("failed to thaw: freezer controller is not available");
    }
    if (auto ret = cgroup_set_value_string(freezeController, "freezer.state", "THAWED"); ret) {
        throw SandboxError("failed to thaw: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::create() {
    if (auto ret = cgroup_create_cgroup(cg_, 0); ret) {
        throw SandboxError("failed to create cgroup: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::attach() {
    if (auto ret = cgroup_attach_task(cg_); ret) {
        throw SandboxError("failed to attach process to cgroup: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::attachTask(pid_t pid) { 
    if (auto ret = cgroup_attach_task_pid(cg_, pid); ret) {
        throw SandboxError("failed to attach process to cgroup: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::loadFromKernel() {
    if (auto ret = cgroup_get_cgroup(cg_); ret) {
        throw SandboxError("failed to read cgroup data from kernel: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::propagateToKernel() {
    if (auto ret = cgroup_modify_cgroup(cg_); ret) {
        throw SandboxError("failed to write data into cgroup in kernel: " + cgroup_strerror(ret));
    }
}

cgroup_controller* CGroupHandler::getController_(const char* name) {
    return cgroup_get_controller(cg_, name);
}

void CGroupHandler::disown() {
    owning_ = false;
}

} // namespace sandbox
