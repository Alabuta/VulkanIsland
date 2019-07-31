#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

#include <string>
using namespace std::string_literals;

#include <nlohmann/json.hpp>

#include "material_loader.hxx"
#include "resources/program.hxx"


namespace
{
std::optional<shader::STAGE> shader_stage_semantic(std::string_view name)
{
    if (name == "vertex"sv)
        return shader::STAGE::VERTEX;

    else if (name == "tesselation_control"sv)
        return shader::STAGE::TESS_CONTROL;

    else if (name == "tesselation_evaluation"sv)
        return shader::STAGE::TESS_EVAL;

    else if (name == "geometry"sv)
        return shader::STAGE::GEOMETRY;

    else if (name == "fragment"sv)
        return shader::STAGE::FRAGMENT;

    else if (name == "compute"sv)
        return shader::STAGE::COMPUTE;

    return { };
}
}

void from_json(nlohmann::json const &j, ShaderStage2 &shader_stage)
{
    shader_stage.moduleName = j.at("name"s).get<std::string>();

    if (auto semantic = shader_stage_semantic(j.at("stage"s).get<std::string>()); semantic)
        shader_stage.semantic = *semantic;

    else throw std::runtime_error("unsupported shder stage"s);
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
        path.replace_extension(".json"sv);

        std::ifstream file{path.native(), std::ios::in};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << '\n';
            return { };
        }

        file >> json;
    }

    auto _name = json.at("name"s).get<std::string>();

    auto shader_modules = json.at("shaderModules"s).get<std::vector<ShaderStage2>>();

    auto material = std::make_shared<Material2>();

    return material;
}
}