#include "exceptions.h"

namespace sandbox
{

SandboxException::SandboxException(std::string msg) : message_{std::move(msg)} 
{}

SandboxException::operator std::string() const {
    return message_;
}

const char* SandboxException::what() const noexcept {
    return message_.c_str();
}

} // namespace sandbox
