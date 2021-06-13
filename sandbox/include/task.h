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

namespace impl {
    int execCmd(void *arg);
    int execWatcher(void *arg);
} // namespace impl

class Task {
public:
    Task() = delete;
    Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints, bool watcherVerbose=false);
    virtual ~Task() = default;

    void start();
    void cancel();
    int await();

    RunAudit getAudit();
    void cleanupImageDir();

protected:
    void exec_();
    void unshare_();
    void prepareImage_();
    void startWatcher_();
    void watcher_();
    void clone_();
    void setNiceness_();
    void prepareMntns_();
    void prepareProcfs_();
    void prepareUserns_(pid_t pid);
    void configureCGroup_();
    void cleanup_();

    void setRealUser_();
    void setEffectiveUser_();

    static std::string generateTaskId_();

    const std::string taskId_;
    const StatusFile statusFile_;

    std::filesystem::path executable_;
    std::vector<std::string> args_;
    TaskConstraints constraints_;
    std::filesystem::path root_;

    std::unique_ptr<CGroupHandler> cgroupHandler_;

    int main2WatcherPipefd_[2];
    int watcher2ExecPipefd_[2];
    pid_t initPid_;
    pid_t taskPid_;
    const bool watcherVerbose_;
    bool interrupted_;

    friend int impl::execCmd(void*);
    friend int impl::execWatcher(void*);
};

} // namespace sandbox

#endif
