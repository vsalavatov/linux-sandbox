#ifndef SANDBOX_TASK_H
#define SANDBOX_TASK_H

#include <vector>
#include <string>
#include <filesystem>

#include "task_constraints.h"
#include "run_audit.h"

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
    void exec_();

    std::filesystem::path executable_;
    std::vector<std::string> args_;
    TaskConstraints constraints_;
    
    pid_t pid_; // todo this is an ad-hoc thingie
};

} // namespace sandbox

#endif
