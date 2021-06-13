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
    int execcmd(void *arg);

    struct Message {
        Message(std::string intro = "(Sandbox) ");
        ~Message();
        std::ostream& operator<<(const auto& o);
    };
};

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
    void exec_();
    void unshare_();
    void prepareImage_();
    void clone_();
    void setNiceness_();
    void prepareMntns_();
    void prepareProcfs_();
    void prepareUserns_();
    void configureCGroup_();

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

    int pipefd_[2];
    pid_t pid_; // todo this is an ad-hoc thingie

    friend int impl::execcmd(void*);
};

} // namespace sandbox

#endif
