#include <iostream>

#include "task.h"
#include "exceptions.h"

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
    "   [--new-network]\n"
    "   [--libcgroup-verbose]";

    std::string executable;
    std::vector<std::string> args;
    std::optional<double> timeLimit;
    std::optional<std::size_t> memoryLimit;
    std::optional<int> niceness;
    std::optional<std::size_t> maxForks;
    bool newNetwork = false;
    bool libcgroupVerbose = false;
    

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
            }
            if (arg == "--libcgroup-verbose") {
                opts.libcgroupVerbose = true;
            }
            if (i >= argc) {
                throw SandboxException(arg + " option without an argument");
            }
            std::stringstream data(argv[i++]);
            if (data.fail()) {
                throw SandboxError(arg + " option expects a numeric argument (bytes)");
            }
            if (arg == "-t" || arg == "--time-limit") {
                throw SandboxException("unsupported argument: " + arg);
            } else if (arg == "-m" || arg == "--memory-limit") {
                size_t limit;
                data >> limit;
                opts.memoryLimit = limit;
            } else if (arg == "-n" || arg == "--niceness") {
                int limit;
                data >> limit;
                opts.niceness = limit;
            } else if (arg == "-f" || arg == "--max-forks") {
                size_t limit;
                data >> limit;
                opts.maxForks = limit;
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

int main(int argc, char *argv[]) {
    Options opts;
    try{
        opts = Options::fromSysArgs(argc, argv);
    } catch(SandboxException &e) {
        std::cout << "Bad arguments: " << e.what() << std::endl;
        std::cout << Options::HELP << std::endl;
        return 0;
    }

    if (opts.libcgroupVerbose) {
        sandbox::setLibCGroupLoggerLevel(100000);
    }

    auto task = Task(
        opts.executable,
        opts.args,
        TaskConstraints{opts.timeLimit, opts.memoryLimit, opts.maxForks,  opts.niceness, opts.newNetwork}
    );

    try {
        task.start();
        task.await();
    } catch (SandboxException &e) {
        std::cout << "Execution failed: " << e.what() << std::endl;
    }

    return 0;
}
