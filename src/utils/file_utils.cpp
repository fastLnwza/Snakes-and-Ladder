#include "file_utils.h"

#include <fstream>
#include <stdexcept>

std::string load_file(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::string contents;
    file.seekg(0, std::ios::end);
    contents.resize(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    return contents;
}

