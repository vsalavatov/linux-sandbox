#include "cgroup_handler.h"
#include "exceptions.h"

#include <unistd.h>

namespace sandbox
{

static void libcgroup_logger_(void *userdata, int level, const char *fmt, va_list ap)
{
    fprintf(stderr, "LIBCGROUP: ");
    vfprintf(stderr, fmt, ap);
}

static void __attribute__ ((constructor)) init_libcgroup() {
    if (int ret = cgroup_init(); ret != 0) {
        fprintf(stderr, "failed to initialize libcgroup: %s\n", cgroup_strerror(ret));
    }
    cgroup_set_logger(libcgroup_logger_, -1, NULL); // set -1 to 100000 to enable verbosity
};

CGroupHandler::CGroupHandler(const char *name) {
    cg_ = cgroup_new_cgroup(name);
    if (!cg_) {
        throw SandboxError("failed to make new cgroup");
    }
}

CGroupHandler::~CGroupHandler() {
    if (cg_) {
        if (int res = cgroup_delete_cgroup(cg_, 0); res) {
            fprintf(stderr, "(Sandbox) Warning: failed to delete cgroup: return code %d", res);
        }
        cgroup_free(&cg_);
    }
}

void CGroupHandler::limitMemory(std::size_t bytes) {
    memory_ = cgroup_add_controller(cg_, "memory");
    if (!memory_) {
        throw SandboxError("failed to initialize cgroup controller \"memory\"");
    }
    if (auto ret = cgroup_set_value_uint64(memory_, "memory.max", bytes); ret) {
        throw SandboxError("failed to set memory limit: " + cgroup_strerror(ret));
    }
    if (auto ret = cgroup_set_value_uint64(memory_, "memory.swap.max", 0); ret) { // disable swap
        throw SandboxError("failed to disable swap: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::create() {
    // TODO: fix this
    /*auto uid = getuid(); 
    auto gid = getgid();
    if (auto ret = cgroup_set_uid_gid(cg_, uid, gid, uid, gid); ret) {
        throw SandboxError("failed to set uid/gid parameters in cgroup: return code " + std::to_string(ret));
    }
    cgroup_set_permissions(cg_, NO_PERMS, NO_PERMS, NO_PERMS);
    */
    
    /*
    freezer_ = cgroup_add_controller(cg_, "freezer");
    if (!freezer_) {
        throw SandboxError("failed to initialize cgroup controller \"freezer\"");
    }
    */

    if (auto ret = cgroup_create_cgroup(cg_, 0); ret) {
        throw SandboxError("failed to create cgroup: " + cgroup_strerror(ret));
    }
}

void CGroupHandler::attach() {
    if (auto ret = cgroup_attach_task(cg_); ret) {
        throw SandboxError("failed to attach process to cgroup: " + cgroup_strerror(ret));
    }
}

} // namespace sandbox
