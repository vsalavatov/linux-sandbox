#include <bits/stdc++.h>
#include <unistd.h>

int main() {
    for (int i = 0; i < 30; i++) {
        std::cout << i << " ";
        std::cout.flush();
        sleep(1);
    }
    std::cout << std::endl << "done" << std::endl;
    return 0;
}
