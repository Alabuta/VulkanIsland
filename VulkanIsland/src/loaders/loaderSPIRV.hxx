#pragma once

#ifdef max
#undef max
#endif

#include <cstddef>
#include <limits>
#include <vector>

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;


namespace loader {
std::vector<std::byte> loadSPIRV(std::string_view name)
{
    fs::path contents{"contents/shaders"sv};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"sv / contents;

    auto path = contents / (std::string{name} +".spv"s);

    std::ifstream file{path.native(), std::ios::in | std::ios::binary};

    if (file.bad() || file.fail())
        return { };

    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());

    std::vector<std::byte> byte_code(static_cast<std::size_t>(file.gcount()));

    file.seekg(start_pos);

    if (!byte_code.empty())
        file.read(reinterpret_cast<char *>(std::data(byte_code)), static_cast<std::streamsize>(std::size(byte_code)));

    return byte_code;
}
}