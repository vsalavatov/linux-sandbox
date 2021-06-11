#include <iostream>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

#include "exceptions.h"
#include "cgroup_handler.h"

using namespace sandbox;
using namespace std::string_literals;

struct Options {
    static constexpr const char* HELP = "" 
    "Arguments format: [--thaw] <status file>";

    std::filesystem::path statusFilePath;
    bool thaw = false;

    static Options fromSysArgs(int argc, char *argv[]) {
        Options opts{};
        if (argc < 2 || argc > 3) {
            throw SandboxException("expected path to the status file.");
        }
        for (int i = 1; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "--thaw") {
                opts.thaw = true;
            } else {
                opts.statusFilePath = std::filesystem::path(arg);
            }
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
    
    pid_t taskPid;
    try {
        if (!std::filesystem::exists(opts.statusFilePath)) {
            throw SandboxException("status file doesn't exist");
        }
        std::fstream statusFile(opts.statusFilePath, std::ios::in);
        statusFile >> taskPid;
        if (statusFile.fail()) {
            throw SandboxException("no pid in status file");
        }
    } catch (SandboxException &e) {
        std::cerr << "Failed to read status file: " << e.what() << std::endl;
        return 1;
    }

    try {
        auto path = "/proc/"s + std::to_string(taskPid) + "/ns/";
        for (auto &nsfile : std::filesystem::directory_iterator(path)) {
            auto nsfilepath = nsfile.path();
            auto filename = nsfilepath.filename();
            if (filename == "user") 
                continue;
            auto fd = open(nsfilepath.c_str(), O_RDONLY | O_CLOEXEC);
            if (fd == -1) {
                throw SandboxException("failed to read namespace link "s + filename.string() + ": "s + std::strerror(errno));
            }
            if (setns(fd, 0) == -1) {
                throw SandboxException("failed to join task's "s + filename.string() + " namespace: "s + std::strerror(errno));
            }
        }
    } catch (SandboxException &e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    CGroupHandler::libinit();
    // CGroupHandler::setLibCGroupLoggerLevel(1000);

    try {
        auto filename = opts.statusFilePath.filename();
        CGroupHandler cg(filename.c_str(), false);
        cg.loadFromKernel();
        if (opts.thaw) {
            cg.thaw();
        } else {
            cg.freeze();
        }
        cg.propagateToKernel();
    } catch (SandboxException &e) {
        std::cout << "CGroup handler failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
