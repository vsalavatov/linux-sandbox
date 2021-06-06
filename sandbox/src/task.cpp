#include "task.h"
#include "exceptions.h"

#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

using namespace std::string_literals;


namespace sandbox
{

Task::Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints)
    : executable_{std::move(executable)}
    , args_{std::move(args)}
    , constraints_{std::move(constraints)}
{}

void Task::start() {
    int pid = fork();
    if (pid < 0) {
        throw SandboxError("failed to fork: "s + std::strerror(errno));
    }
    if (pid != 0) { // parent
        pid_ = pid;
        return;
    }
    // child

    std::vector<const char*> argv(1 + args_.size() + 1);
    argv[0] = executable_.c_str();
    for (auto i = 0; i < args_.size(); i++) {
        argv[1 + i] = args_[i].c_str();
    }

    auto res = execvp(executable_.c_str(), const_cast<char* const*>(argv.data()));
    if (res < 0) {
        throw SandboxException("failed to start the task: "s + std::strerror(errno));
    }
    // started
}

void Task::cancel() {

}

void Task::await() {
    auto r = waitpid(pid_, nullptr, 0);
    if (r < 0) {
        throw SandboxException("failed to await the task: "s + std::strerror(errno));
    }
}

RunAudit Task::getAudit() {

}

} // namespace sandbox
