#include "task.h"

int main(int argc, char *argv[]) {
    sandbox::Task("./kek", {"1"}, sandbox::TaskConstraints{std::nullopt, std::nullopt, std::nullopt});
    return 0;
}
