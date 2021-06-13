#ifndef SANDBOX_STATUS_FILE_H
#define SANDBOX_STATUS_FILE_H

#include <filesystem>

namespace sandbox
{
    
class StatusFile {
public:
    StatusFile() = delete;
    StatusFile(std::filesystem::path path);
    ~StatusFile();

private:
    std::filesystem::path path_;
};

} // namespace sandbox


#endif
