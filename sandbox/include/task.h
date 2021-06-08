#ifndef SANDBOX_TASK_H
#define SANDBOX_TASK_H

#include <vector>
#include <string>
#include <filesystem>
#include <memory>

#include "task_constraints.h"
#include "run_audit.h"
#include "cgroup_handler.h"

namespace sandbox
{

class Task {
public:
    Task() = delete;
    Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints);
    virtual ~Task() = default;

    void start();
    void cancel();
    void await();

    RunAudit getAudit();

protected:
    void prepare_();
    void unshare_();
    void configure_cgroup_();
    void exec_();

    static std::string generateTaskId_();

    std::filesystem::path executable_;
    std::vector<std::string> args_;
    TaskConstraints constraints_;

    std::unique_ptr<CGroupHandler> cgroupHandler_;
    
    pid_t pid_; // todo this is an ad-hoc thingie
};

} // namespace sandbox

#endif
