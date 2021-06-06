#include <iostream>

#include "task.h"
#include "exceptions.h"

using namespace sandbox;

struct Options {
    static constexpr const char* HELP = "" 
    "Arguments format: "
    "[-t|--time-limit <seconds>] [-m|--memory-limit <bytes>] -- <executable> <arguments...>";

    std::string executable;
    std::vector<std::string> args;

    // todo add constraints

    static Options fromSysArgs(int argc, char *argv[]) {
        Options opts;
        int i = 1;
        while (i < argc) {
            std::string arg(argv[i]);
            i++;
            if (arg == "--") break;
            // todo parse contraints
            if (arg == "-t" || arg == "--time-limit") {

            } else if (arg == "-m" || arg == "--memory-limit") {

            } else {
                throw SandboxException("unsupported argument " + arg);
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

    auto task = Task(
        opts.executable, 
        opts.args, 
        TaskConstraints{std::nullopt, std::nullopt, std::nullopt} // todo 
    );

    try {
        task.start();
        task.await();
    } catch (SandboxException &e) {
        std::cout << "Execution failed: " << e.what() << std::endl;
    }
    
    
    return 0;
}
