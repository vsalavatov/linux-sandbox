#ifndef SANDBOX_TASK_H
#define SANDBOX_TASK_H

#include <vector>
#include <string>
#include <filesystem>
#include <memory>

#include "task_constraints.h"
#include "run_audit.h"
#include "cgroup_handler.h"
#include "status_file.h"

namespace sandbox
{

class Task {
public:
    Task() = delete;
    Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints);
    virtual ~Task() = default;

    void prepare();
    void cancel();
    void await();
    void exec();

    RunAudit getAudit();

protected:
    void unshare_();
    void clone_();
    void prepareMntns_(std::string rootfs);
    void prepareProcfs_();
    void prepareUserns_();
    void configureCGroup_();

    static std::string generateTaskId_();

    const std::string taskId_;
    const StatusFile statusFile_;

    std::filesystem::path executable_;
    std::vector<std::string> args_;
    TaskConstraints constraints_;

    std::unique_ptr<CGroupHandler> cgroupHandler_;

    int pipefd_[2];
    pid_t pid_; // todo this is an ad-hoc thingie
};

} // namespace sandbox

#endif
