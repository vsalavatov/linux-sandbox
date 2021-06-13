#include "task.h"
#include "exceptions.h"

#include <sys/resource.h>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <random>

#define STACKSIZE (1024*1024)
static char cmd_stack[STACKSIZE];

using namespace std::string_literals;


namespace sandbox
{

Task::Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints)
    : taskId_{generateTaskId_()}
    , statusFile_{taskId_}
    , executable_{std::move(executable)}
    , args_{std::move(args)}
    , constraints_{std::move(constraints)}
{
}

void Task::start() {
    prepare_();
}

void Task::cancel() {
    throw SandboxError("not implemented");
}

void Task::await() { // this is an ad-hod impl
    int status;
    auto r = waitpid(pid_, &status, 0);
    if (r < 0) {
        throw SandboxException("failed to await the task: "s + std::strerror(errno));
    }
    if (WIFEXITED(status)) {
        std::cerr << "exited with code: " << WEXITSTATUS(status) << std::endl;
    }
    if (WIFSIGNALED(status)) {
        std::cerr << "terminated by signal: " << WTERMSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")" << std::endl;
    }
    if (WIFSTOPPED(status)) {
        std::cerr << "stopped by signal: " << WSTOPSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")" << std::endl;
    }
}

static int execcmd(void* arg) {
    Task *task = ((Task*)arg);
    task->exec_();
    return 1;
}

void Task::prepare_() {
    if (pipe(pipefd) < 0)
        throw SandboxError("Failed to create pipe: " + strerror(errno));
    clone_();
    configure_cgroup_();
    if (write(pipefd[1], "OK", 2) != 2)
        throw SandboxError("failed to write to pipe: " + strerror(errno));
    if (close(pipefd[1]))
        throw SandboxError("failed to close pipe: " + strerror(errno));
}

void Task::clone_() {
    int flags = SIGCHLD | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC;
    if (constraints_.newNetwork) {
        flags |= CLONE_NEWNET;
    }
    pid_ = clone(execcmd, cmd_stack + STACKSIZE, flags, this);
    if (pid_ == -1)
        throw SandboxError("failed to clone: " + strerror(errno));

}

void Task::configure_cgroup_() {
    cgroupHandler_ = std::make_unique<CGroupHandler>(taskId_.c_str());
    
    if (constraints_.maxMemoryBytes) {
        cgroupHandler_->limitMemory(*constraints_.maxMemoryBytes);
    }
    if (constraints_.maxForks) {
        cgroupHandler_->limitProcesses(*constraints_.maxForks);
    }
    if (constraints_.freezable) {
        cgroupHandler_->addFreezerController();
    }
    cgroupHandler_->create();
}

void Task::exec_() {
    char buf[2];
    if (read(pipefd[0], buf, 2) != 2)
        throw SandboxError("failed to read from pipe: " + strerror(errno));
    if (constraints_.niceness) {
        int ret = setpriority(PRIO_PROCESS, 0, *constraints_.niceness);
        if (ret == -1) {
            throw SandboxException("failed to set niceness: "s + strerror(errno));
        }
    }
    std::vector<const char*> argv(1 + args_.size() + 1);
    argv[0] = executable_.c_str();
    for (auto i = 0; i < args_.size(); i++) {
        argv[1 + i] = args_[i].c_str();
    }

    cgroupHandler_->attach();

    auto res = execvp(executable_.c_str(), const_cast<char* const*>(argv.data()));
    if (res < 0) {
        throw SandboxException("failed to start the task: "s + std::strerror(errno));
    }
}

RunAudit Task::getAudit() {
    throw SandboxError("not implemented");
}


std::string Task::generateTaskId_() {
    static const std::string alphabet = "0123456789abcdef";
    static const int length = 8;
    
    static std::mt19937 gen(time(nullptr));
    
    std::string result;
    for (int i = 0; i < length; i++) {
        result += alphabet[gen() % alphabet.length()];
    }
    return "sandbox-task-"s + result;
}

} // namespace sandbox
