#include <iostream>
#include <signal.h>

#include "task.h"
#include "exceptions.h"
#include "msg.h"

using namespace sandbox;

struct Options {
    static constexpr const char* HELP = "" 
    "Arguments format:"
    "[options]... -- <executable> <arguments...>\n"
    "Options:\n"
    "   [-t|--time-limit <seconds>]\n"
    "   [-m|--memory-limit <bytes>]\n"
    "   [-f|--max-forks <count>]\n"
    "   [-n|--niceness <value from [-20, 19]>]\n"
    "   [--no-freezer]\n"
    "   [--new-network]\n"
    "   [--libcgroup-verbose]\n"
    "   [--watcher-verbose]\n"
    "   [-i|--fs-image <path> [-a|--add <path-from>:<path-to>]...]\n"
    "   [-w|--work-dir <path>]\n"
    "   [-u|--uid <uid>]\n"
    "   [-g|--gid <gid>]\n";

    std::string executable;
    std::vector<std::string> args;
    std::optional<double> timeLimit;
    std::optional<std::size_t> memoryLimit;
    std::optional<int> niceness;
    std::optional<std::size_t> maxForks;
    bool newNetwork = false;
    bool libcgroupVerbose = false;
    bool watcherVerbose = false;
    bool enableFreezer = true;
    std::optional<std::filesystem::path> fsImage;
    std::filesystem::path workDir = ".";
    std::vector<TaskConstraints::FileMapping> fileMapping;
    uid_t uid = 1000;
    gid_t gid = 1000;

    static Options fromSysArgs(int argc, char *argv[]) {
        Options opts{};
        int i = 1;
        while (i < argc) {
            std::string arg(argv[i]);
            i++;
            if (arg == "--") break;
            // todo parse contraints
            if (arg == "--new-network") {
                opts.newNetwork = true;
                continue;
            }
            if (arg == "--libcgroup-verbose") {
                opts.libcgroupVerbose = true;
                continue;
            }
            if (arg == "--watcher-verbose") {
                opts.watcherVerbose = true;
                continue;
            }
            if (arg == "--no-freezer") {
                opts.enableFreezer = false;
                continue;
            }
            if (i >= argc) {
                throw SandboxException(arg + " option without an argument");
            }
            std::stringstream data(argv[i++]);
            auto onReadFail = [&](std::string expected) {
                if (data.fail()) {
                    throw SandboxException(arg + " option expects " + expected);
                }
            };
            if (arg == "-t" || arg == "--time-limit") {
                throw SandboxException("unsupported argument: " + arg);
            } else if (arg == "-m" || arg == "--memory-limit") {
                size_t limit;
                data >> limit;
                onReadFail("a numeric argument (# bytes)");
                opts.memoryLimit = limit;
            } else if (arg == "-n" || arg == "--niceness") {
                int limit;
                data >> limit;
                onReadFail("a numeric argument (niceness)");
                opts.niceness = limit;
            } else if (arg == "-f" || arg == "--max-forks") {
                size_t limit;
                data >> limit;
                onReadFail("a numeric argument (# forks)");
                opts.maxForks = limit;
            } else if (arg == "-i" || arg == "--fs-image") {
                std::filesystem::path p;
                data >> p;
                onReadFail("a path to the container image");
                opts.fsImage = p;
            } else if (arg == "-w" || arg == "--work-dir") {
                std::filesystem::path p;
                data >> p;
                onReadFail("a path of working directory inside the container (or host fs)");
                opts.workDir = p;
            } else if (arg == "-a" || arg == "--add") {
                std::string pp;
                data >> pp;
                onReadFail(arg + " expects <path-from>:<path-to>, e.g. ./bin/cmd:/app/cmd");
                auto it = pp.find(':');
                if (it == std::string::npos) {
                    throw SandboxException(arg + " expects <path-from>:<path-to>, e.g. ./bin/cmd:/app/cmd");
                }
                std::filesystem::path from{pp.substr(0, it)}, to{pp.substr(it + 1)};
                opts.fileMapping.emplace_back(from, to);
            } else if (arg == "-u" || arg == "--uid") {
                uid_t uid;
                data >> uid;
                onReadFail("expected uid");
                opts.uid = uid;
            } else if (arg == "-g" || arg == "--gid") {
                uid_t gid;
                data >> gid;
                onReadFail("expected gid");
                opts.gid = gid;
            } else {
                throw SandboxException("unsupported argument: " + arg);
            }
        }
        if (i >= argc) throw SandboxException("no executable is specified");
        opts.executable = std::string(argv[i++]);
        while (i < argc) {
            opts.args.emplace_back(std::string(argv[i++]));
        }
        return opts;
    }
};

static std::unique_ptr<Task> task;
void sighandler(int sig) {
    if (task) {
        task->cancel();
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sighandler);

    Options opts;
    try{
        opts = Options::fromSysArgs(argc, argv);
    } catch(SandboxException &e) {
        std::cout << "Bad arguments: " << e.what() << std::endl;
        std::cout << Options::HELP << std::endl;
        return 1;
    }

    CGroupHandler::libinit();

    if (opts.libcgroupVerbose) {
        CGroupHandler::setLibCGroupLoggerLevel(100000);
    }

    task = std::make_unique<Task>(
        opts.executable,
        opts.args,
        TaskConstraints{
            opts.timeLimit, 
            opts.memoryLimit, 
            opts.maxForks, 
            opts.niceness, 
            opts.newNetwork, 
            opts.enableFreezer,
            opts.fsImage,
            opts.workDir,
            opts.fileMapping,
            opts.uid,
            opts.gid
        },
        opts.watcherVerbose
    );

    try {
        task->start();
        return task->await();
    } catch (SandboxException &e) {
        impl::Message() << "Execution failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
