#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <fstream>

int main() {
    pid_t pid = getpid();
    std::string path = "/proc/" + std::to_string(pid) + "/mounts";
    std::fstream fin(std::filesystem::path(path), std::ios::in);
    std::string line;
    while (std::getline(fin, line)) {
        std::cout << line << std::endl;
    }
    return 0;
}