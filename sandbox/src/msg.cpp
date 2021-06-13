#include "msg.h"

namespace sandbox
{

impl::Message::Message(std::string intro) {
    std::cerr << "\u001b[33;1m" << intro;
}

impl::Message::~Message() {
    std::cerr << "\u001b[0m" << std::endl;
}
    
} // namespace sandbox

