#include <vector>
#include <optional>

#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <string>
using namespace std::string_literals;

#include <nlohmann/json.hpp>

#include "material_loader.hxx"



namespace loader
{
    std::optional<graphics::SHADER_STAGE> shader_stage_semantic(std::string_view name)
    {
        if (name == "vertex"sv)
            return graphics::SHADER_STAGE::VERTEX;

        else if (name == "tesselation_control"sv)
            return graphics::SHADER_STAGE::TESS_CONTROL;

        else if (name == "tesselation_evaluation"sv)
            return graphics::SHADER_STAGE::TESS_EVAL;

        else if (name == "geometry"sv)
            return graphics::SHADER_STAGE::GEOMETRY;

        else if (name == "fragment"sv)
            return graphics::SHADER_STAGE::FRAGMENT;

        else if (name == "compute"sv)
            return graphics::SHADER_STAGE::COMPUTE;

        return { };
    }

    std::optional<vertex::attribute_semantic> attribute_semantic(std::string_view name)
    {
        if (name == "POSITION"sv)
            return vertex::position{ };

        else if (name == "NORMAL"sv)
            return vertex::normal{ };

        else if (name == "TEXCOORD_0"sv)
            return vertex::tex_coord_0{ };

        else if (name == "TEXCOORD_1"sv)
            return vertex::tex_coord_1{ };

        else if (name == "TANGENT"sv)
            return vertex::tangent{ };

        else if (name == "COLOR_0"sv)
            return vertex::color_0{ };

        else if (name == "JOINTS_0"sv)
            return vertex::joints_0{ };

        else if (name == "WEIGHTS_0"sv)
            return vertex::weights_0{ };

        return { };
    }

    std::optional<vertex::attribute_type> attribute_type(std::string_view type)
    {
        if (type == "float"sv)
            return vertex::static_array<1, boost::float32_t>{};

        else if (type == "vec2"sv)
            return vertex::static_array<2, boost::float32_t>{};

        else if (type == "vec3"sv)
            return vertex::static_array<3, boost::float32_t>{};

        else if (type == "vec4"sv)
            return vertex::static_array<4, boost::float32_t>{};

        else if (type == "int"sv)
            return vertex::static_array<1, std::int32_t>{};

        else if (type == "ivec2"sv)
            return vertex::static_array<2, std::int32_t>{};

        else if (type == "ivec3"sv)
            return vertex::static_array<3, std::int32_t>{};

        else if (type == "ivec4"sv)
            return vertex::static_array<4, std::int32_t>{};

        else if (type == "uint"sv)
            return vertex::static_array<1, std::uint32_t>{};

        else if (type == "uvec2"sv)
            return vertex::static_array<2, std::uint32_t>{};

        else if (type == "uvec3"sv)
            return vertex::static_array<3, std::uint32_t>{};

        else if (type == "uvec4"sv)
            return vertex::static_array<4, std::uint32_t>{};

        return { };
    }

    std::optional<graphics::CULL_MODE> cull_mode(std::string_view name)
    {
        if (name == "none"sv)
            return graphics::CULL_MODE::NONE;

        else if (name == "front"sv)
            return graphics::CULL_MODE::FRONT;

        else if (name == "back"sv)
            return graphics::CULL_MODE::BACK;

        else if (name == "front_and_back"sv)
            return graphics::CULL_MODE::FRONT_AND_BACK;

        return { };
    }

    std::optional<graphics::POLYGON_MODE> polygon_mode(std::string_view name)
    {
        if (name == "fill"sv)
            return graphics::POLYGON_MODE::FILL;

        else if (name == "line"sv)
            return graphics::POLYGON_MODE::LINE;

        else if (name == "point"sv)
            return graphics::POLYGON_MODE::POINT;

        return { };
    }

    std::optional<graphics::COMPARE_OPERATION> compare_operation(std::string_view name)
    {
        if (name == "never"sv)
            return graphics::COMPARE_OPERATION::NEVER;

        else if (name == "less"sv)
            return graphics::COMPARE_OPERATION::LESS;

        else if (name == "equal"sv)
            return graphics::COMPARE_OPERATION::EQUAL;

        else if (name == "less_or_equal"sv)
            return graphics::COMPARE_OPERATION::LESS_OR_EQUAL;

        else if (name == "greater"sv)
            return graphics::COMPARE_OPERATION::GREATER;

        else if (name == "not_equal"sv)
            return graphics::COMPARE_OPERATION::NOT_EQUAL;

        else if (name == "greater_or_equal"sv)
            return graphics::COMPARE_OPERATION::GREATER_OR_EQUAL;

        else if (name == "always"sv)
            return graphics::COMPARE_OPERATION::ALWAYS;

        return { };
    }

    std::optional<graphics::STENCIL_OPERATION> stencil_operation(std::string_view name)
    {
        if (name == "keep"sv)
            return graphics::STENCIL_OPERATION::KEEP;

        else if (name == "zero"sv)
            return graphics::STENCIL_OPERATION::ZERO;

        else if (name == "replace"sv)
            return graphics::STENCIL_OPERATION::REPLACE;

        else if (name == "increment_and_clamp"sv)
            return graphics::STENCIL_OPERATION::INCREMENT_AND_CLAMP;

        else if (name == "decrement_and_clamp"sv)
            return graphics::STENCIL_OPERATION::DECREMENT_AND_CLAMP;

        else if (name == "invert"sv)
            return graphics::STENCIL_OPERATION::INVERT;

        else if (name == "increment_and_wrap"sv)
            return graphics::STENCIL_OPERATION::INCREMENT_AND_WRAP;

        else if (name == "decrement_and_wrap"sv)
            return graphics::STENCIL_OPERATION::DECREMENT_AND_WRAP;

        return { };
    }

    std::optional<graphics::LOGIC_OPERATION> logic_operation(std::string_view name)
    {
        if (name == "clear"sv)
            return graphics::LOGIC_OPERATION::CLEAR;

        else if (name == "and"sv)
            return graphics::LOGIC_OPERATION::AND;

        else if (name == "and_reverse"sv)
            return graphics::LOGIC_OPERATION::AND_REVERSE;

        else if (name == "copy"sv)
            return graphics::LOGIC_OPERATION::COPY;

        else if (name == "and_inverted"sv)
            return graphics::LOGIC_OPERATION::AND_INVERTED;

        else if (name == "no_op"sv)
            return graphics::LOGIC_OPERATION::NO_OP;

        else if (name == "xor"sv)
            return graphics::LOGIC_OPERATION::XOR;

        else if (name == "or"sv)
            return graphics::LOGIC_OPERATION::OR;

        else if (name == "nor"sv)
            return graphics::LOGIC_OPERATION::NOR;

        else if (name == "equivalent"sv)
            return graphics::LOGIC_OPERATION::EQUIVALENT;

        else if (name == "invert"sv)
            return graphics::LOGIC_OPERATION::INVERT;

        else if (name == "OR_REVERSE"sv)
            return graphics::LOGIC_OPERATION::OR_REVERSE;

        else if (name == "copy_inverted"sv)
            return graphics::LOGIC_OPERATION::COPY_INVERTED;

        else if (name == "or_inverted"sv)
            return graphics::LOGIC_OPERATION::OR_INVERTED;

        else if (name == "nand"sv)
            return graphics::LOGIC_OPERATION::NAND;

        else if (name == "set"sv)
            return graphics::LOGIC_OPERATION::SET;

        return { };
    }

    std::optional<graphics::BLEND_FACTOR> blend_factor(std::string_view name)
    {
        if (name == "zero"sv)
            return graphics::BLEND_FACTOR::ZERO;

        else if (name == "one"sv)
            return graphics::BLEND_FACTOR::ONE;

        else if (name == "src_color"sv)
            return graphics::BLEND_FACTOR::SRC_COLOR;

        else if (name == "one_minus_src_color"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_SRC_COLOR;

        else if (name == "dst_color"sv)
            return graphics::BLEND_FACTOR::DST_COLOR;

        else if (name == "one_minus_dst_color"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_DST_COLOR;

        else if (name == "src_alpha"sv)
            return graphics::BLEND_FACTOR::SRC_ALPHA;

        else if (name == "one_minus_src_alpha"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;

        else if (name == "dst_alpha"sv)
            return graphics::BLEND_FACTOR::DST_ALPHA;

        else if (name == "one_minus_dst_alpha"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_DST_ALPHA;

        else if (name == "constant_color"sv)
            return graphics::BLEND_FACTOR::CONSTANT_COLOR;

        else if (name == "one_minus_constant_color"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR;

        else if (name == "constant_alpha"sv)
            return graphics::BLEND_FACTOR::CONSTANT_ALPHA;

        else if (name == "one_minus_constant_alpha"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA;

        else if (name == "src_alpha_saturate"sv)
            return graphics::BLEND_FACTOR::SRC_ALPHA_SATURATE;

        else if (name == "src1_color"sv)
            return graphics::BLEND_FACTOR::SRC1_COLOR;

        else if (name == "one_minus_src1_color"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_SRC1_COLOR;

        else if (name == "src1_alpha"sv)
            return graphics::BLEND_FACTOR::SRC1_ALPHA;

        else if (name == "one_minus_src1_alpha"sv)
            return graphics::BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA;

        return { };
    }

    std::optional<graphics::BLEND_OPERATION> blend_operation(std::string_view name)
    {
        if (name == "add"sv)
            return graphics::BLEND_OPERATION::ADD;

        else if (name == "subtract"sv)
            return graphics::BLEND_OPERATION::SUBTRACT;

        return { };
    }

    std::optional<graphics::COLOR_COMPONENT> color_component(std::string_view name)
    {
        if (name == "red"sv)
            return graphics::COLOR_COMPONENT::R;

        else if (name == "green"sv)
            return graphics::COLOR_COMPONENT::G;

        else if (name == "blue"sv)
            return graphics::COLOR_COMPONENT::B;

        else if (name == "alpha"sv)
            return graphics::COLOR_COMPONENT::A;

        else if (name == "rgb"sv)
            return graphics::COLOR_COMPONENT::RGB;

        else if (name == "rgba"sv)
            return graphics::COLOR_COMPONENT::RGBA;

        return { };
    }

    std::optional<material_description::specialization_constant> specialization_constant_value(std::string_view type, float value)
    {
        if (type == "float"sv)
            return static_cast<boost::float32_t>(value);

        else if (type == "int"sv)
            return static_cast<std::int32_t>(value);

        return { };
    }

}

namespace nlohmann
{
    void from_json(nlohmann::json const &j, loader::material_description::shader_module &shader_module)
    {
        if (auto stage = loader::shader_stage_semantic(j.at("stage"s).get<std::string>()); stage)
            shader_module.stage = *stage;

        else throw std::runtime_error("unsupported shader stage"s);

        shader_module.name = j.at("name"s).get<std::string>();
    }

    void from_json(nlohmann::json const &j, loader::material_description::vertex_attribute &vertex_attribute)
    {
        if (auto semantic = loader::attribute_semantic(j.at("semantic"s).get<std::string>()); semantic)
            vertex_attribute.semantic = *semantic;

        else throw std::runtime_error("unsupported vertex attribute semantic"s);

        if (auto type = loader::attribute_type(j.at("type"s).get<std::string>()); type)
            vertex_attribute.type = *type;

        else throw std::runtime_error("unsupported vertex attribute type"s);
    }

    void from_json(nlohmann::json const &j, loader::material_description::specialization_constant &specialization_constant)
    {
        auto type = j.at("type"s).get<std::string>();

        if (auto value = loader::specialization_constant_value(type, j.at("value"s).get<float>()); value)
            specialization_constant = *value;

        else throw std::runtime_error("unsupported specialization constant value"s);
    }

    void from_json(nlohmann::json const &j, loader::material_description::shader_bundle &shader_bundle)
    {
        shader_bundle.module_index = j.at("index"s).get<std::size_t>();
        shader_bundle.technique_index = j.at("technique"s).get<std::size_t>();

        using specialization_constants = std::vector<loader::material_description::specialization_constant>;

        if (j.count("specializationConstants"s))
            shader_bundle.specialization_constants = j.at("specializationConstants"s).get<specialization_constants>();
    }

    void from_json(nlohmann::json const &j, loader::material_description::technique &technique)
    {
        technique.shaders_bundle = j.at("shadersBundle"s).get<std::vector<loader::material_description::shader_bundle>>();

        technique.vertex_layout = j.at("vertexLayout"s).get<std::vector<std::size_t>>();
    }
}

namespace loader
{
    loader::material_description load_material_description(std::string_view name)
    {
        nlohmann::json json;

        {
            fs::path contents{"contents/materials"s};

            if (!fs::exists(fs::current_path() / contents))
                contents = fs::current_path() / "../"s / contents;

            auto path = (contents / name).replace_extension(".json"sv);

            std::ifstream file{path.native(), std::ios::in};

            if (file.bad() || file.fail()) {
                std::cerr << "failed to open file: "s << path << '\n';
                return { };
            }

            file >> json;
        }

        auto shader_modules = json.at("shaderModules"s).get<std::vector<material_description::shader_module>>();

        auto techniques = json.at("techniques"s).get<std::vector<material_description::technique>>();

        auto vertex_attributes = json.at("vertexAttributes"s).get<std::vector<material_description::vertex_attribute>>();

        return loader::material_description{
            std::string{name},
            shader_modules,
            vertex_attributes,
            techniques
        };
    }
}