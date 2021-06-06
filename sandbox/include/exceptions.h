#ifndef SANDBOX_EXCEPTIONS_H
#define SANDBOX_EXCEPTIONS_H

#include <string>
#include <stdexcept>


namespace sandbox
{

class SandboxException : std::exception {
public:
    #define SandboxError(msg) sandbox::SandboxException(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + msg)
    explicit SandboxException(std::string msg);

    operator std::string() const;
    const char* what() const noexcept override;

private:
    std::string message_;
};

} // namespace sandbox


#endif
