#include "task.h"
#include "exceptions.h"

#include <sys/resource.h>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <syscall.h>
#include <iostream>
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
    if (pipe(pipefd_) < 0)
        throw SandboxError("Failed to create pipe: "s + strerror(errno));
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
    return 0;
}

void Task::prepare_() {
    if (pipe(pipefd_) < 0)
        throw SandboxError("Failed to create pipe: " + strerror(errno));
    configure_cgroup_();
    clone_();
    cgroupHandler_->attachTask(pid_);
    prepareUserns_();
    if (write(pipefd_[1], "OK", 2) != 2)
        throw SandboxError("failed to write to pipe: " + strerror(errno));
    if (close(pipefd_[1]))
        throw SandboxError("failed to close pipe: " + strerror(errno));
}

void Task::clone_() {
    int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWUSER;
    if (constraints_.newNetwork) {
        flags |= CLONE_NEWNET;
    }
    pid_ = clone(execcmd, cmd_stack + STACKSIZE, flags, this);
    if (pid_ == -1)
        throw SandboxError("failed to clone: " + strerror(errno));
}

void Task::prepareProcfs_() {
    if (mkdir("/proc", 0555) && errno != EEXIST)
        throw SandboxError("failed to mkdir /proc: "s + strerror(errno));

    if (mount("proc", "/proc", "proc", 0, ""))
        throw SandboxError("failed to mount proc: "s + strerror(errno));
}

void Task::prepareMntns_(std::string rootfs) {
    std::string mnt = rootfs + "_mnt";
    if (mount(rootfs.c_str(), mnt.c_str(), "ext4", MS_BIND, ""))
        throw SandboxError("failed to mount " + rootfs + " at " + mnt + ": " + strerror(errno));

    if (chdir(mnt.c_str()))
        throw SandboxError("failed to chdir to rootfs mounted at " + mnt + ": " + strerror(errno));

    std::string put_old = "put_old";
    if (mkdir(put_old.c_str(), 0777) && errno != EEXIST)
        throw SandboxError("failed to mkdir " + put_old + ": " + strerror(errno));

    std::cout << "Current path is " << std::filesystem::current_path() << '\n';

    if (syscall(SYS_pivot_root, ".", put_old.c_str()))
        throw SandboxError("failed to pivot_root from . to " + put_old + ": " + strerror(errno));

    if (chdir("/"))
        throw SandboxError("failed to chdir to new root: "s + strerror(errno));

    prepareProcfs_();

    if (umount2(put_old.c_str(), MNT_DETACH))
        throw SandboxError("failed to umount " + put_old + ": " + strerror(errno));
}


void write_file(char path[100], char line[100]) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        throw SandboxError("Failed to open file " + path + " :" + strerror(errno));
    }
    if (fwrite(line, 1, strlen(line), f) < 0) {
        throw SandboxError("Failed to write to file " + path + " :" + strerror(errno));
    }
    if (fclose(f) != 0) {
        throw SandboxError("Failed to close file " + path + " :" + strerror(errno));
    }
}

void Task::prepareUserns_() {
    char path[100];
    char line[100];

    int uid = 1000;

    sprintf(path, "/proc/%d/uid_map", pid_);
    sprintf(line, "0 %d 1\n", uid);
    write_file(path, line);

    sprintf(path, "/proc/%d/setgroups", pid_);
    sprintf(line, "deny");
    write_file(path, line);

    sprintf(path, "/proc/%d/gid_map", pid_);
    sprintf(line, "0 %d 1\n", uid);
    write_file(path, line);
}

void Task::configure_cgroup_() {
    if (unshare(CLONE_NEWCGROUP)) {
        throw SandboxError("failed to unshare cgroup: "s + std::strerror(errno));
    }
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
    // We're done once we read something from the pipe.
    char buf[2];
    if (read(pipefd_[0], buf, 2) != 2)
        throw SandboxError("failed to read from pipe: "s + strerror(errno));

    // Assuming, 0 in the current namespace maps to
    // a non-privileged UID in the parent namespace,
    // drop superuser privileges if any by enforcing
    // the exec'ed process runs with UID 0.
    if (setgid(0) == -1)
        throw SandboxError("failed to setgid: "s + strerror(errno));
    if (setuid(0) == -1)
        throw SandboxError("failed to setuid: "s + strerror(errno));


    if (constraints_.niceness) {
        int ret = setpriority(PRIO_PROCESS, 0, *constraints_.niceness);
        if (ret == -1) {
            throw SandboxError("failed to set niceness: "s + strerror(errno));
        }
    }
    std::vector<const char*> argv(1 + args_.size() + 1);
    argv[0] = executable_.c_str();
    for (auto i = 0; i < args_.size(); i++) {
        argv[1 + i] = args_[i].c_str();
    }

    cgroupHandler_->attach();

    prepareMntns_("../rootfs");
    auto res = execvp(executable_.c_str(), const_cast<char* const*>(argv.data()));
    if (res < 0) {
        throw SandboxError("failed to start the task: "s + std::strerror(errno));
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
