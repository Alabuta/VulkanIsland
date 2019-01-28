#include "program.hxx"


[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view name)
{
    fs::path contents{"shaders"sv};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"sv / contents;

    auto path = contents / name;

    std::ifstream file{path.native(), std::ios::in | std::ios::binary};

    if (file.bad() || file.fail())
        return { };

    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());

    std::vector<std::byte> shaderByteCode(static_cast<std::size_t>(file.gcount()));

    file.seekg(start_pos);

    if (!shaderByteCode.empty())
        file.read(reinterpret_cast<char *>(std::data(shaderByteCode)), static_cast<std::streamsize>(std::size(shaderByteCode)));

    return shaderByteCode;
}
