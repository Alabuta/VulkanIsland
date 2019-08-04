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

std::optional<COMPARE_OPERATION> compare_operation(std::string_view name)
{
    if (name == "never"sv)
        return COMPARE_OPERATION::NEVER;

    else if (name == "less"sv)
        return COMPARE_OPERATION::LESS;

    else if (name == "equal"sv)
        return COMPARE_OPERATION::EQUAL;

    else if (name == "less_or_equal"sv)
        return COMPARE_OPERATION::LESS_OR_EQUAL;

    else if (name == "greater"sv)
        return COMPARE_OPERATION::GREATER;

    else if (name == "not_equal"sv)
        return COMPARE_OPERATION::NOT_EQUAL;

    else if (name == "greater_or_equal"sv)
        return COMPARE_OPERATION::GREATER_OR_EQUAL;

    else if (name == "always"sv)
        return COMPARE_OPERATION::ALWAYS;

    return { };
}

std::optional<BLEND_STATE_OPERATION> logic_operation(std::string_view name)
{
    if (name == "clear"sv)
        return BLEND_STATE_OPERATION::CLEAR;

    else if (name == "and"sv)
        return BLEND_STATE_OPERATION::AND;

    else if (name == "and_reverse"sv)
        return BLEND_STATE_OPERATION::AND_REVERSE;

    else if (name == "copy"sv)
        return BLEND_STATE_OPERATION::COPY;

    else if (name == "and_inverted"sv)
        return BLEND_STATE_OPERATION::AND_INVERTED;

    else if (name == "no_op"sv)
        return BLEND_STATE_OPERATION::NO_OP;

    else if (name == "xor"sv)
        return BLEND_STATE_OPERATION::XOR;

    else if (name == "or"sv)
        return BLEND_STATE_OPERATION::OR;

    else if (name == "nor"sv)
        return BLEND_STATE_OPERATION::NOR;

    else if (name == "equivalent"sv)
        return BLEND_STATE_OPERATION::EQUIVALENT;

    else if (name == "invert"sv)
        return BLEND_STATE_OPERATION::INVERT;

    else if (name == "OR_REVERSE"sv)
        return BLEND_STATE_OPERATION::OR_REVERSE;

    else if (name == "copy_inverted"sv)
        return BLEND_STATE_OPERATION::COPY_INVERTED;

    else if (name == "or_inverted"sv)
        return BLEND_STATE_OPERATION::OR_INVERTED;

    else if (name == "nand"sv)
        return BLEND_STATE_OPERATION::NAND;

    else if (name == "set"sv)
        return BLEND_STATE_OPERATION::SET;

    return { };
}

std::optional<BLEND_FACTOR> blend_factor(std::string_view name)
{
    if (name == "zero"sv)
        return BLEND_FACTOR::ZERO;

    else if (name == "one"sv)
        return BLEND_FACTOR::ONE;

    else if (name == "src_color"sv)
        return BLEND_FACTOR::SRC_COLOR;

    else if (name == "one_minus_src_color"sv)
        return BLEND_FACTOR::ONE_MINUS_SRC_COLOR;

    else if (name == "dst_color"sv)
        return BLEND_FACTOR::DST_COLOR;

    else if (name == "one_minus_dst_color"sv)
        return BLEND_FACTOR::ONE_MINUS_DST_COLOR;

    else if (name == "src_alpha"sv)
        return BLEND_FACTOR::SRC_ALPHA;

    else if (name == "one_minus_src_alpha"sv)
        return BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;

    else if (name == "dst_alpha"sv)
        return BLEND_FACTOR::DST_ALPHA;

    else if (name == "one_minus_dst_alpha"sv)
        return BLEND_FACTOR::ONE_MINUS_DST_ALPHA;

    else if (name == "constant_color"sv)
        return BLEND_FACTOR::CONSTANT_COLOR;

    else if (name == "one_minus_constant_color"sv)
        return BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR;

    else if (name == "constant_alpha"sv)
        return BLEND_FACTOR::CONSTANT_ALPHA;

    else if (name == "one_minus_constant_alpha"sv)
        return BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA;

    else if (name == "src_alpha_saturate"sv)
        return BLEND_FACTOR::SRC_ALPHA_SATURATE;

    else if (name == "src1_color"sv)
        return BLEND_FACTOR::SRC1_COLOR;

    else if (name == "one_minus_src1_color"sv)
        return BLEND_FACTOR::ONE_MINUS_SRC1_COLOR;

    else if (name == "src1_alpha"sv)
        return BLEND_FACTOR::SRC1_ALPHA;

    else if (name == "one_minus_src1_alpha"sv)
        return BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA;

    return { };
}

std::optional<BLEND_OPERATION> blend_operation(std::string_view name)
{
    if (name == "add"sv)
        return BLEND_OPERATION::ADD;

    else if (name == "subtract"sv)
        return BLEND_OPERATION::SUBTRACT;

    return { };
}

std::optional<COLOR_COMPONENT> color_component(std::string_view name)
{
    if (name == "red"sv)
        return COLOR_COMPONENT::R;

    else if (name == "green"sv)
        return COLOR_COMPONENT::G;

    else if (name == "blue"sv)
        return COLOR_COMPONENT::B;

    else if (name == "alpha"sv)
        return COLOR_COMPONENT::A;

    else if (name == "rgb"sv)
        return COLOR_COMPONENT::RGB;

    else if (name == "rgba"sv)
        return COLOR_COMPONENT::RGBA;

    return { };
}
}


void from_json(nlohmann::json const &j, material_description::shader_module &shader_module)
{
    if (auto semantic = loader::shader_stage_semantic(j.at("stage"s).get<std::string>()); semantic)
        shader_module.semantic = *semantic;

    else throw std::runtime_error("unsupported shader stage"s);

    shader_module.name = j.at("name"s).get<std::string>();
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
    shader_bundle.module_index = j.at("index"s).get<std::size_t>();
    shader_bundle.technique_index = j.at("technique"s).get<std::size_t>();
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

void from_json(nlohmann::json const &j, material_description::depth_stencil_state &depth_stencil_state)
{
    if (auto compare_operation = loader::compare_operation(j.at("depthCompareOperation"s).get<std::string>()); compare_operation)
        depth_stencil_state.depth_compare_operation = *compare_operation;

    else throw std::runtime_error("unsupported depth compare operation"s);

    depth_stencil_state.depth_test_enable = j.at("depthTestEnable"s).get<bool>();
    depth_stencil_state.depth_write_enable = j.at("depthWriteEnable"s).get<bool>();
    depth_stencil_state.stencil_test_enable = j.at("stencilTestEnable"s).get<bool>();
}

void from_json(nlohmann::json const &j, material_description::color_blend_state &color_blend_state)
{
    if (auto logic_operation = loader::logic_operation(j.at("logicOperation"s).get<std::string>()); logic_operation)
        color_blend_state.logic_operation = *logic_operation;

    else throw std::runtime_error("unsupported logic operation"s);

    color_blend_state.logic_operation_enable = j.at("logicOperationEnable"s).get<bool>();

    color_blend_state.blend_constants = j.at("blendConstants"s).get<std::array<float, 4>>();

    color_blend_state.attachments = j.at("attachments"s).get<std::vector<std::size_t>>();
}

void from_json(nlohmann::json const &j, material_description::color_blend_attachment_state &color_blend_attachment_state)
{
    if (auto blend_factor = loader::blend_factor(j.at("srcColorBlendFactor"s).get<std::string>()); blend_factor)
        color_blend_attachment_state.src_color_blend_factor = *blend_factor;

    else throw std::runtime_error("unsupported source color blend factor"s);

    if (auto blend_factor = loader::blend_factor(j.at("dstColorBlendFactor"s).get<std::string>()); blend_factor)
        color_blend_attachment_state.dst_color_blend_factor = *blend_factor;

    else throw std::runtime_error("unsupported destination color blend factor"s);

    if (auto blend_operation = loader::blend_operation(j.at("colorBlendOperation"s).get<std::string>()); blend_operation)
        color_blend_attachment_state.color_blend_operation = *blend_operation;

    else throw std::runtime_error("unsupported color blend factor"s);

    if (auto blend_factor = loader::blend_factor(j.at("srcAlphaBlendFactor"s).get<std::string>()); blend_factor)
        color_blend_attachment_state.src_alpha_blend_factor = *blend_factor;

    else throw std::runtime_error("unsupported source alpha blend factor"s);

    if (auto blend_factor = loader::blend_factor(j.at("dstAlphaBlendFactor"s).get<std::string>()); blend_factor)
        color_blend_attachment_state.dst_alpha_blend_factor = *blend_factor;

    else throw std::runtime_error("unsupported destination alpha blend factor"s);

    if (auto blend_operation = loader::blend_operation(j.at("alphaBlendOperation"s).get<std::string>()); blend_operation)
        color_blend_attachment_state.alpha_blend_operation = *blend_operation;

    else throw std::runtime_error("unsupported alpha blend factor"s);

    if (auto color_component = loader::color_component(j.at("colorWriteMask"s).get<std::string>()); color_component)
        color_blend_attachment_state.color_write_mask = *color_component;

    else throw std::runtime_error("unsupported color component"s);

    color_blend_attachment_state.blend_enable = j.at("blendEnable"s).get<bool>();
}


namespace loader
{
std::shared_ptr<material_description> load_material_description(std::string_view name)
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

    auto shader_modules = json.at("shaderModules"s).get<std::vector<material_description::shader_module>>();

    auto vertex_attributes = json.at("vertexAttributes"s).get<std::vector<material_description::vertex_attribute>>();

    auto techniques = json.at("techniques"s).get<std::vector<material_description::technique>>();

    auto rasterization_states = json.at("rasterizationStates"s).get<std::vector<material_description::rasterization_state>>();
    auto depth_stencil_states = json.at("depthStencilStates"s).get<std::vector<material_description::depth_stencil_state>>();

    auto color_blend_states = json.at("colorBlendStates"s).get<std::vector<material_description::color_blend_state>>();
    auto attachment_states = json.at("colorBlendAttachmentStates"s).get<std::vector<material_description::color_blend_attachment_state>>();

    return std::make_shared<material_description>(material_description{
        std::string{name},
        shader_modules,
        vertex_attributes,
        techniques,
        rasterization_states,
        depth_stencil_states,
        color_blend_states,
        attachment_states
    });
}
}