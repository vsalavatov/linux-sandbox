#ifndef SANDBOX_MSG_H
#define SANDBOX_MSG_H

#include <string>
#include <iostream>

namespace sandbox
{
    
namespace impl
{

struct Message {
    Message(std::string intro = "(Sandbox) ");
    ~Message();

    inline std::ostream& operator<<(const auto& o) {
        std::cerr << o;
        return std::cerr;
    }
};

} // namespace impl

} // namespace sandbox


#endif
