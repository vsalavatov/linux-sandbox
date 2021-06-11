#include <unistd.h>
#include <cstring>
#include <iostream>

int main() {
    for (int i = 0; i < 100; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            std::cout << "Failed to fork (i = " << i << "): " << strerror(errno) << std::endl;
            break;
        }
        if (pid == 0) {
            sleep(1);
            return 0;
        }
    }
    return 0;
}