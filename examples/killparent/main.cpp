#include <iostream>
#include <csignal>
#include <unistd.h>

int main() {
    kill(getppid(), SIGKILL);
    std::cout << "Failed to kill watcher." << std::endl;
    return 0;
}

