#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

#include <string>
using namespace std::string_literals;

#include <nlohmann/json.hpp>

#include "renderer/attachments.hxx"
#include "resources/program.hxx"
#include "vertexFormat.hxx"
#include "material_loader.hxx"



struct material_description final {
    std::string name;

    struct shader_module final {
        shader::STAGE semantic;
        std::string name;
    };

    std::vector<shader_module> shader_modules;

    struct vertex_attribute final {
        semantics_t semantic;
        attribute_t type;
    };

    std::vector<vertex_attribute> vertex_attributes;

    struct shader_bundle final {
        std::size_t index;
        std::size_t technique;
    };

    struct technique final {
        std::vector<shader_bundle> shaders_bundle;
        std::vector<std::size_t> vertex_layout;

        std::size_t rasterization_state;
        std::size_t depth_stencil_state;
        std::size_t color_blend_state;
    };

    std::vector<technique> techniques;

    struct rasterization_state final {
        CULL_MODE cull_mode{CULL_MODE::BACK};
        POLYGON_FRONT_FACE front_face{POLYGON_FRONT_FACE::COUNTER_CLOCKWISE};
        POLYGON_MODE polygon_mode{POLYGON_MODE::FILL};

        float line_width{1.f};
    };

    std::vector<rasterization_state> rasterization_states;

    struct depth_stencil_state final {
        bool depth_test_enable{true};
        bool depth_write_enable{true};

        COMPARE_OPERATION depth_compare_operation{COMPARE_OPERATION::GREATER};

        bool stencil_test_enable{false};
    };

    std::vector<depth_stencil_state> depth_stencil_states;

    struct color_blend_state final {
        bool logic_operation_enable{false};

        BLEND_STATE_OPERATION logic_operation{BLEND_STATE_OPERATION::COPY};

        std::array<float, 4> blend_constants{0, 0, 0, 0};

        std::vector<std::size_t> attachments;
    };

    std::vector<color_blend_state> color_blend_states;

    struct color_blend_attachment_state final {
        bool blend_enable{false};

        BLEND_FACTOR src_color_blend_factor{BLEND_FACTOR::ONE};
        BLEND_FACTOR dst_color_blend_factor{BLEND_FACTOR::ZERO};

        BLEND_OPERATION color_blend_operation{BLEND_OPERATION::ADD};

        BLEND_FACTOR src_alpha_blend_factor{BLEND_FACTOR::ONE};
        BLEND_FACTOR dst_alpha_blend_factor{BLEND_FACTOR::ZERO};

        BLEND_OPERATION alpha_blend_operation{BLEND_OPERATION::ADD};

        COLOR_COMPONENT color_write_mask{COLOR_COMPONENT::RGBA};
    };

    std::vector<color_blend_attachment_state> color_blend_attachment_states;
};


namespace loader
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

std::optional<semantics_t> attribute_semantic(std::string_view name)
{
    if (name == "POSITION"sv)
        return semantic::position{ };

    else if (name == "NORMAL"sv)
        return semantic::normal{ };

    else if (name == "TEXCOORD_0"sv)
        return semantic::tex_coord_0{ };

    else if (name == "TEXCOORD_1"sv)
        return semantic::tex_coord_1{ };

    else if (name == "TANGENT"sv)
        return semantic::tangent{ };

    else if (name == "COLOR_0"sv)
        return semantic::color_0{ };

    else if (name == "JOINTS_0"sv)
        return semantic::joints_0{ };

    else if (name == "WEIGHTS_0"sv)
        return semantic::weights_0{ };

    return { };
}

std::optional<attribute_t> attribute_type(std::string_view type)
{
    if (type == "float"sv)
        return vec<1, float>{};

    else if (type == "vec2"sv)
        return vec<2, float>{};

    else if (type == "vec3"sv)
        return vec<3, float>{};

    else if (type == "vec4"sv)
        return vec<4, float>{};

    else if (type == "int"sv)
        return vec<1, std::int32_t>{};

    else if (type == "ivec2"sv)
        return vec<2, std::int32_t>{};

    else if (type == "ivec3"sv)
        return vec<3, std::int32_t>{};

    else if (type == "ivec4"sv)
        return vec<4, std::int32_t>{};

    else if (type == "uint"sv)
        return vec<1, std::uint32_t>{};

    else if (type == "uvec2"sv)
        return vec<2, std::uint32_t>{};

    else if (type == "uvec3"sv)
        return vec<3, std::uint32_t>{};

    else if (type == "uvec4"sv)
        return vec<4, std::uint32_t>{};

    return { };
}

std::optional<CULL_MODE> cull_mode(std::string_view name)
{
    if (name == "none"sv)
        return CULL_MODE::NONE;

    else if (name == "front"sv)
        return CULL_MODE::FRONT;

    else if (name == "back"sv)
        return CULL_MODE::BACK;

    else if (name == "front_and_back"sv)
        return CULL_MODE::FRONT_AND_BACK;

    return { };
}

std::optional<POLYGON_MODE> polygon_mode(std::string_view name)
{
    if (name == "fill"sv)
        return POLYGON_MODE::FILL;

    else if (name == "line"sv)
        return POLYGON_MODE::LINE;

    else if (name == "point"sv)
        return POLYGON_MODE::POINT;

    return { };
}
}

void from_json(nlohmann::json const &j, material_description::shader_module &shader_module)
{

    if (auto semantic = loader::shader_stage_semantic(j.at("stage"s).get<std::string>()); semantic)
        shader_module.semantic = *semantic;

    else throw std::runtime_error("unsupported shader stage"s);
}

void from_json(nlohmann::json const &j, material_description::vertex_attribute &vertex_attribute)
{
    if (auto semantic = loader::attribute_semantic(j.at("semantic"s).get<std::string>()); semantic)
        vertex_attribute.semantic = *semantic;

    else throw std::runtime_error("unsupported vertex attribute semantic"s);

    if (auto type = loader::attribute_type(j.at("type"s).get<std::string>()); type)
        vertex_attribute.type = *type;

    else throw std::runtime_error("unsupported vertex attribute type"s);
}

void from_json(nlohmann::json const &j, material_description::shader_bundle &shader_bundle)
{
    shader_bundle.index = j.at("index"s).get<std::size_t>();
    shader_bundle.technique = j.at("technique"s).get<std::size_t>();
}

void from_json(nlohmann::json const &j, material_description::technique &technique)
{
    technique.shaders_bundle = j.at("shadersBundle"s).get<std::vector<material_description::shader_bundle>>();

    technique.vertex_layout = j.at("vertexLayout"s).get<std::vector<std::size_t>>();

    technique.rasterization_state = j.at("rasterizationState"s).get<std::size_t>();
    technique.depth_stencil_state = j.at("depthStencilState"s).get<std::size_t>();
    technique.color_blend_state = j.at("colorBlendState"s).get<std::size_t>();
}

void from_json(nlohmann::json const &j, material_description::rasterization_state &rasterization_state)
{
    if (auto cull_mode = loader::cull_mode(j.at("cullMode"s).get<std::string>()); cull_mode)
        rasterization_state.cull_mode = *cull_mode;

    else throw std::runtime_error("unsupported cull mode"s);

    if (auto polygon_mode = loader::polygon_mode(j.at("polygonMode"s).get<std::string>()); polygon_mode)
        rasterization_state.polygon_mode = *polygon_mode;

    else throw std::runtime_error("unsupported polygon mode"s);

    auto front_face_clockwise = j.at("frontFaceClockwise"s).get<bool>();
    rasterization_state.front_face = front_face_clockwise ? POLYGON_FRONT_FACE::CLOCKWISE : POLYGON_FRONT_FACE::COUNTER_CLOCKWISE;
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

    auto shader_modules = json.at("shaderModules"s).get<std::vector<material_description::shader_module>>();
    auto vertex_attributes = json.at("vertexAttributes"s).get<std::vector<material_description::vertex_attribute>>();
    auto techniques = json.at("techniques"s).get<std::vector<material_description::technique>>();
    auto rasterization_states = json.at("rasterizationStates"s).get<std::vector<material_description::rasterization_state>>();

    auto material = std::make_shared<Material2>();

    return material;
}
}