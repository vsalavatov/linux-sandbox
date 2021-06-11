#include "status_file.h"

#include <fstream>

namespace sandbox
{

StatusFile::StatusFile(std::filesystem::path path) : path_{path} 
{
    std::fstream f(path_, std::ios::out);
    f.close();
}

StatusFile::~StatusFile() {
    if (std::filesystem::exists(path_)) {
        if (!std::filesystem::remove(path_)) {
            fprintf(stderr, "(Sandbox) Warning: failed to delete status file %s\n", path_.c_str());
        }
    }
}

} // namespace sandbox
