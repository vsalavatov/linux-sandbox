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

#define STACKSIZE (64*1024*1024)
static char cmd_stack[STACKSIZE];

using namespace std::string_literals;


namespace sandbox
{

impl::Message::Message(std::string intro) {
    std::cerr << "\u001b[33;1m" << intro;
}

impl::Message::~Message() {
    std::cerr << "\u001b[0m" << std::endl;
}

std::ostream& impl::Message::operator<<(const auto& o) {
    std::cerr << o;
    return std::cerr;
}


Task::Task(std::filesystem::path executable, std::vector<std::string> args, TaskConstraints constraints, bool watcherVerbose)
    : taskId_{generateTaskId_()}
    , statusFile_{taskId_}
    , executable_{std::move(executable)}
    , args_{std::move(args)}
    , constraints_{std::move(constraints)}
    , root_{"/"}
    , watcherVerbose_{watcherVerbose}
{
}

void Task::cancel() {
    throw SandboxError("not implemented");
}

int Task::await() { 
    int status;
    auto pid = waitpid(initPid_, &status, 0);
    if (pid < 0) {
        throw SandboxException("failed to await the task: "s + std::strerror(errno));
    }
    if (WIFEXITED(status)) {
        impl::Message() << "exited with code: " << WEXITSTATUS(status);
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        impl::Message() << "terminated by signal: " << WTERMSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")";
    }
    if (WIFSTOPPED(status)) {
        impl::Message() << "stopped by signal: " << WSTOPSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")";
    }
    return 72;
}

void Task::start() {
    impl::Message() << "Starting task " << taskId_ << "...";
    if (pipe(main2WatcherPipefd_) < 0 || pipe(watcher2ExecPipefd_) < 0)
        throw SandboxError("failed to create pipe: " + strerror(errno));
    unshare_();
    configureCGroup_();
    prepareImage_();
    startWatcher_();
    cgroupHandler_->attachTask(initPid_);
    setNiceness_();
    prepareUserns_(initPid_);
    if (write(main2WatcherPipefd_[1], "OK", 2) != 2)
        throw SandboxError("failed to write to pipe: " + strerror(errno));
    if (close(main2WatcherPipefd_[1]))
        throw SandboxError("failed to close pipe: " + strerror(errno));
}

void Task::unshare_() {
    int flags = CLONE_NEWCGROUP;
    if (constraints_.newNetwork) {
        flags |= CLONE_NEWNET;
    }
    if (unshare(flags)) {
        throw SandboxError("failed to unshare namespaces: "s + std::strerror(errno));
    }
}

void Task::prepareImage_() {
    if (constraints_.fsImage == std::nullopt) 
        return;
    impl::Message() << "Preparing image...";

    root_ = std::filesystem::absolute(taskId_ + ".d/");
    auto copyOpts = std::filesystem::copy_options{std::filesystem::copy_options::recursive};
    try {
        std::filesystem::create_directories(root_);
        copy(*constraints_.fsImage, root_, copyOpts);
    } catch (std::exception &e) {
        throw SandboxException("failed to copy fs image: "s + e.what());
    }
    for (auto &m : constraints_.fileMapping) {
        try {
            if (std::filesystem::is_directory(m.from)) {
                std::filesystem::create_directories(m.to);
            } else {
                std::filesystem::create_directories(m.to.parent_path());
            }
            auto fp = m.to.string();
            if (fp.starts_with("/")) fp = fp.substr(1);
            copy(m.from, root_ / fp, copyOpts);
        } catch (std::exception &e) {
            throw SandboxException("failed to copy "s + m.from.string() + " to "s + (m.to).string() + ": "s + e.what());
        }
    }
    if (chown(root_.c_str(), constraints_.uid, constraints_.gid)) {
        throw SandboxError("failed to chown image: "s + std::strerror(errno));
    }
    impl::Message() << "Image is ready";
}

void Task::startWatcher_() {
    int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWUSER;
    initPid_ = clone(impl::execWatcher, cmd_stack + STACKSIZE, flags, this);
    if (initPid_ == -1)
        throw SandboxError("failed to start watcher: " + strerror(errno));
}

void Task::watcher_() {
    char buf[2];
    if (read(main2WatcherPipefd_[0], buf, 2) != 2)
        throw SandboxError("failed to read from pipe: "s + strerror(errno));
    if (close(main2WatcherPipefd_[0])) 
        throw SandboxError("failed to close pipe: "s + strerror(errno));
    clone_();
    int retcode = 71;
    int status;
    pid_t pid;
    while (true) {
        pid = waitpid(-1, &status, 0);
        if (errno == ECHILD) break;
        if (pid < 0) {
            throw SandboxException("(watcher) failed to await the task: "s + std::strerror(errno));
        }
        if (pid == taskPid_ && WIFEXITED(status)) {
            retcode = WEXITSTATUS(status);
        }
        if (WIFEXITED(status) && watcherVerbose_) {
            impl::Message() << "(watcher) pid " << pid << " exited with code: " << WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status) && watcherVerbose_) {
            impl::Message() << "(watcher) pid " << pid << " terminated by signal: " << WTERMSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")";
        }
        if (WIFSTOPPED(status) && watcherVerbose_) {
            impl::Message() << "(watcher) pid " << pid << " stopped by signal: " << WSTOPSIG(status) << " (" << strsignal(WTERMSIG(status)) << ")";
        }
    }
    exit(retcode);
}

int impl::execCmd(void* arg) {
    Task *task = ((Task*)arg);
    try {
        task->exec_();
    } catch (SandboxException &e) {
        impl::Message() << e.what();
        return 69;
    }
    return 0;
}

int impl::execWatcher(void* arg) {
    Task *task = ((Task*)arg);
    try {
        task->watcher_();
    } catch (SandboxException &e) {
        impl::Message() << e.what();
        return 70;
    }
    return 0;
}

void Task::clone_() {
    int flags = SIGCHLD | CLONE_NEWNS | CLONE_NEWIPC;
    taskPid_ = clone(impl::execCmd, cmd_stack + STACKSIZE, flags, this);
    if (taskPid_ == -1)
        throw SandboxError("failed to clone: " + strerror(errno));
    // prepareUserns_(pid_);
    if (write(watcher2ExecPipefd_[1], "OK", 2) != 2)
        throw SandboxError("failed to write to pipe: " + strerror(errno));
    if (close(watcher2ExecPipefd_[1]))
        throw SandboxError("failed to close pipe: " + strerror(errno));
}

void Task::setNiceness_() {
    if (constraints_.niceness) {
        int ret = setpriority(PRIO_PROCESS, initPid_, *constraints_.niceness);
        if (ret == -1) {
            throw SandboxError("failed to set niceness: "s + strerror(errno));
        }
    }
}

void Task::prepareProcfs_() {
    if (mkdir("/proc", 0555) && errno != EEXIST)
        throw SandboxError("failed to mkdir /proc: "s + strerror(errno));

    if (mount("proc", "/proc", "proc", 0, ""))
        throw SandboxError("failed to mount proc: "s + strerror(errno));
}

void Task::prepareMntns_() {
    if (constraints_.fsImage == std::nullopt) 
        return;
    std::string mnt = root_;
    if (mount(root_.c_str(), mnt.c_str(), "ext4", MS_BIND, ""))
        throw SandboxError("failed to mount image at " + mnt + ": " + strerror(errno));

    if (chdir(mnt.c_str()))
        throw SandboxError("failed to chdir to image mounted at " + mnt + ": " + strerror(errno));
    
    std::string put_old = "put_old";
    if (mkdir(put_old.c_str(), 0777) && errno != EEXIST)
        throw SandboxError("failed to mkdir " + put_old + ": " + strerror(errno));

    if (syscall(SYS_pivot_root, ".", put_old.c_str()))
        throw SandboxError("failed to pivot_root from . to " + put_old + ": " + strerror(errno));

    if (chdir("/"))
        throw SandboxError("failed to chdir to new root: "s + strerror(errno));

    prepareProcfs_();

    if (umount2(put_old.c_str(), MNT_DETACH))
        throw SandboxError("failed to umount " + put_old + ": " + strerror(errno));

    if (chdir(constraints_.workDir.c_str()))
        throw SandboxError("failed to chdir to working directory: " + strerror(errno));
}


void write_file(char path[100], char line[100]) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        throw SandboxError("failed to open file " + path + " :" + std::strerror(errno));
    }
    if (fwrite(line, 1, strlen(line), f) < 0) {
        throw SandboxError("failed to write to file " + path + " :" + std::strerror(errno));
    }
    if (fclose(f) != 0) {
        throw SandboxError("failed to close file " + path + " :" + std::strerror(errno));
    }
}

void Task::prepareUserns_(pid_t pid) {
    char path[100];
    char line[100];

    uid_t uid = constraints_.uid;
    gid_t gid = constraints_.gid;

    sprintf(path, "/proc/%d/uid_map", pid);
    sprintf(line, "0 %d 1\n", uid);
    write_file(path, line);

    sprintf(path, "/proc/%d/setgroups", pid);
    sprintf(line, "deny");
    write_file(path, line);

    sprintf(path, "/proc/%d/gid_map", pid);
    sprintf(line, "0 %d 1\n", gid);
    write_file(path, line);
}

void Task::configureCGroup_() {
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
    if (read(watcher2ExecPipefd_[0], buf, 2) != 2)
        throw SandboxError("failed to read from pipe: "s + strerror(errno));
    if (close(watcher2ExecPipefd_[0])) 
        throw SandboxError("failed to close pipe: "s + strerror(errno));

    if (setgid(0) == -1)
        throw SandboxError("failed to setgid: "s + strerror(errno));
    if (setuid(0) == -1)
        throw SandboxError("failed to setuid: "s + strerror(errno));

    prepareMntns_();

    std::vector<const char*> argv(1 + args_.size() + 1);
    argv[0] = executable_.c_str();
    for (auto i = 0; i < args_.size(); i++) {
        argv[1 + i] = args_[i].c_str();
    }
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
