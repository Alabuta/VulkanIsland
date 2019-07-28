#include <filesystem>
namespace fs = std::filesystem;

#include <string>
using namespace std::string_literals;

#include <nlohmann/json.hpp>

#include "material_loader.hxx"
#include "resources/program.hxx"


namespace
{
void from_json(nlohmann::json const &j, ShaderStage2 &shader_stage)
{
    /*buffer.byteLength = j.at("byteLength"s).get<decltype(buffer_t::byteLength)>();
    buffer.uri = j.at("uri"s).get<decltype(buffer_t::uri)>();*/
}
}

namespace loader
{
std::shared_ptr<Material2> load_material(std::string_view name)
{
    nlohmann::json json;

    {
        fs::path contents{"contents/materials"s};

        if (!fs::exists(fs::current_path() / contents))
            contents = fs::current_path() / "../"s / contents;

        auto path = contents / name;

        std::ifstream file{path.native(), std::ios::in};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << '\n';
            return false;
        }

        file >> json;
    }

    auto shader_stages = json.at("shaderModules"s).get<std::vector<ShaderStage2>>();
}
}