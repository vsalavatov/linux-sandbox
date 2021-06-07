#include <bits/stdc++.h>

#include "httplib.h"

int main() {
    httplib::Client client("https://ifconfig.me");
    auto result = client.Get("/ip");
    std::cout << result->status << std::endl;
    std::cout << result->body << std::endl;
    return 0;
}
