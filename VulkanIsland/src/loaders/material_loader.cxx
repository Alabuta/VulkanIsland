#include <vector>
#include <optional>
#include <unordered_map>

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
        static const std::unordered_map<std::string_view, graphics::SHADER_STAGE> stages{
            {"vertex"sv, graphics::SHADER_STAGE::VERTEX},
            {"tesselation_control"sv, graphics::SHADER_STAGE::TESS_CONTROL},
            {"tesselation_evaluation"sv, graphics::SHADER_STAGE::TESS_EVAL},
            {"geometry"sv, graphics::SHADER_STAGE::GEOMETRY},
            {"fragment"sv, graphics::SHADER_STAGE::FRAGMENT},
            {"compute"sv, graphics::SHADER_STAGE::COMPUTE}
        };

        if (auto it = stages.find(name); it != std::end(stages))
            return it->second;

        return { };
    }

    std::optional<vertex::attribute_semantic> attribute_semantic(std::string_view name)
    {
        static const std::unordered_map<std::string_view, vertex::attribute_semantic> semantics{
            {"POSITION"sv, vertex::position{ }},
            {"NORMAL"sv, vertex::normal{ }},
            {"TEXCOORD_0"sv, vertex::tex_coord_0{ }},
            {"TEXCOORD_1"sv, vertex::tex_coord_1{ }},
            {"TANGENT"sv, vertex::tangent{ }},
            {"COLOR_0"sv, vertex::color_0{ }},
            {"JOINTS_0"sv, vertex::joints_0{ }},
            {"WEIGHTS_0"sv, vertex::weights_0{ }}
        };

        if (auto it = semantics.find(name); it != std::end(semantics))
            return it->second;

        return { };
    }

    std::optional<graphics::FORMAT> attribute_format(std::string_view type)
    {
        static const std::unordered_map<std::string_view, graphics::FORMAT> formats{
            {"r8snorm"sv, graphics::FORMAT::R8_SNORM},
            {"rg8snorm"sv, graphics::FORMAT::RG8_SNORM},
            {"rgb8snorm"sv, graphics::FORMAT::RGB8_SNORM},
            {"rgba8snorm"sv, graphics::FORMAT::RGBA8_SNORM},

            {"r32sint"sv, graphics::FORMAT::R32_SINT},
            {"rg32sint"sv, graphics::FORMAT::RG32_SINT},
            {"rgb32sint"sv, graphics::FORMAT::RGB32_SINT},
            {"rgba32sint"sv, graphics::FORMAT::RGBA32_SINT},

            {"r32uint"sv, graphics::FORMAT::R32_UINT},
            {"rg32uint"sv, graphics::FORMAT::RG32_UINT},
            {"rgb32uint"sv, graphics::FORMAT::RGB32_UINT},
            {"rgba32uint"sv, graphics::FORMAT::RGBA32_UINT},

            {"r32sfloat"sv, graphics::FORMAT::R32_SFLOAT},
            {"rg32sfloat"sv, graphics::FORMAT::RG32_SFLOAT},
            {"rgb32sfloat"sv, graphics::FORMAT::RGB32_SFLOAT},
            {"rgba32sfloat"sv, graphics::FORMAT::RGBA32_SFLOAT}
        };

        if (auto it = formats.find(type); it != std::end(formats))
            return it->second;

        return { };
    }

    std::optional<vertex::attribute_type> attribute_type(std::string_view type)
    {
        if (type == "rg8snorm"sv)
            return vertex::static_array<2, boost::int8_t>{};

        else if (type == "r32sfloat"sv)
            return vertex::static_array<1, boost::float32_t>{};

        else if (type == "rg32sfloat"sv)
            return vertex::static_array<2, boost::float32_t>{};

        else if (type == "rgb32sfloat"sv)
            return vertex::static_array<3, boost::float32_t>{};

        else if (type == "rgba32sfloat"sv)
            return vertex::static_array<4, boost::float32_t>{};

        else if (type == "r32sint"sv)
            return vertex::static_array<1, std::int32_t>{};

        else if (type == "rg32sint"sv)
            return vertex::static_array<2, std::int32_t>{};

        else if (type == "rgb32sint"sv)
            return vertex::static_array<3, std::int32_t>{};

        else if (type == "rgba32sint"sv)
            return vertex::static_array<4, std::int32_t>{};

        else if (type == "r32uint"sv)
            return vertex::static_array<1, std::uint32_t>{};

        else if (type == "rg32uint"sv)
            return vertex::static_array<2, std::uint32_t>{};

        else if (type == "rgb32uint"sv)
            return vertex::static_array<3, std::uint32_t>{};

        else if (type == "rgba32uint"sv)
            return vertex::static_array<4, std::uint32_t>{};

        return { };
    }

    std::optional<graphics::CULL_MODE> cull_mode(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::CULL_MODE> modes{
            {"none"sv, graphics::CULL_MODE::NONE},
            {"front"sv, graphics::CULL_MODE::FRONT},
            {"back"sv, graphics::CULL_MODE::BACK},
            {"front_and_back"sv, graphics::CULL_MODE::FRONT_AND_BACK}
        };

        if (auto it = modes.find(name); it != std::end(modes))
            return it->second;

        return { };
    }

    std::optional<graphics::POLYGON_MODE> polygon_mode(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::POLYGON_MODE> modes{
            {"fill"sv, graphics::POLYGON_MODE::FILL},
            {"line"sv, graphics::POLYGON_MODE::LINE},
            {"point"sv, graphics::POLYGON_MODE::POINT}
        };

        if (auto it = modes.find(name); it != std::end(modes))
            return it->second;

        return { };
    }

    std::optional<graphics::COMPARE_OPERATION> compare_operation(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::COMPARE_OPERATION> operations{
            {"never"sv, graphics::COMPARE_OPERATION::NEVER},
            {"less"sv, graphics::COMPARE_OPERATION::LESS},
            {"equal"sv, graphics::COMPARE_OPERATION::EQUAL},
            {"less_or_equal"sv, graphics::COMPARE_OPERATION::LESS_OR_EQUAL},
            {"greater"sv, graphics::COMPARE_OPERATION::GREATER},
            {"not_equal"sv, graphics::COMPARE_OPERATION::NOT_EQUAL},
            {"greater_or_equal"sv, graphics::COMPARE_OPERATION::GREATER_OR_EQUAL},
            {"always"sv, graphics::COMPARE_OPERATION::ALWAYS}
        };

        if (auto it = operations.find(name); it != std::end(operations))
            return it->second;

        return { };
    }

    std::optional<graphics::STENCIL_OPERATION> stencil_operation(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::STENCIL_OPERATION> operations{
            {"keep"sv, graphics::STENCIL_OPERATION::KEEP},
            {"zero"sv, graphics::STENCIL_OPERATION::ZERO},
            {"replace"sv, graphics::STENCIL_OPERATION::REPLACE},
            {"increment_and_clamp"sv, graphics::STENCIL_OPERATION::INCREMENT_AND_CLAMP},
            {"decrement_and_clamp"sv, graphics::STENCIL_OPERATION::DECREMENT_AND_CLAMP},
            {"invert"sv, graphics::STENCIL_OPERATION::INVERT},
            {"increment_and_wrap"sv, graphics::STENCIL_OPERATION::INCREMENT_AND_WRAP},
            {"decrement_and_wrap"sv, graphics::STENCIL_OPERATION::DECREMENT_AND_WRAP}
        };

        if (auto it = operations.find(name); it != std::end(operations))
            return it->second;

        return { };
    }

    std::optional<graphics::LOGIC_OPERATION> logic_operation(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::LOGIC_OPERATION> operations{
            {"clear"sv, graphics::LOGIC_OPERATION::CLEAR},
            {"and"sv, graphics::LOGIC_OPERATION::AND},
            {"and_reverse"sv, graphics::LOGIC_OPERATION::AND_REVERSE},
            {"copy"sv, graphics::LOGIC_OPERATION::COPY},
            {"and_inverted"sv, graphics::LOGIC_OPERATION::AND_INVERTED},
            {"no_op"sv, graphics::LOGIC_OPERATION::NO_OP},
            {"xor"sv, graphics::LOGIC_OPERATION::XOR},
            {"or"sv, graphics::LOGIC_OPERATION::OR},
            {"nor"sv, graphics::LOGIC_OPERATION::NOR},
            {"equivalent"sv, graphics::LOGIC_OPERATION::EQUIVALENT},
            {"invert"sv, graphics::LOGIC_OPERATION::INVERT},
            {"OR_REVERSE"sv, graphics::LOGIC_OPERATION::OR_REVERSE},
            {"copy_inverted"sv, graphics::LOGIC_OPERATION::COPY_INVERTED},
            {"or_inverted"sv, graphics::LOGIC_OPERATION::OR_INVERTED},
            {"nand"sv, graphics::LOGIC_OPERATION::NAND},
            {"set"sv, graphics::LOGIC_OPERATION::SET}
        };

        if (auto it = operations.find(name); it != std::end(operations))
            return it->second;

        return { };
    }

    std::optional<graphics::BLEND_FACTOR> blend_factor(std::string_view name)
    {
        static const std::unordered_map<std::string_view, graphics::BLEND_FACTOR> factors{
            {"zero"sv, graphics::BLEND_FACTOR::ZERO},
            {"one"sv, graphics::BLEND_FACTOR::ONE},
            {"src_color"sv, graphics::BLEND_FACTOR::SRC_COLOR},
            {"one_minus_src_color"sv, graphics::BLEND_FACTOR::ONE_MINUS_SRC_COLOR},
            {"dst_color"sv, graphics::BLEND_FACTOR::DST_COLOR},
            {"one_minus_dst_color"sv, graphics::BLEND_FACTOR::ONE_MINUS_DST_COLOR},
            {"src_alpha"sv, graphics::BLEND_FACTOR::SRC_ALPHA},
            {"one_minus_src_alpha"sv, graphics::BLEND_FACTOR::ONE_MINUS_SRC_ALPHA},
            {"dst_alpha"sv, graphics::BLEND_FACTOR::DST_ALPHA},
            {"one_minus_dst_alpha"sv, graphics::BLEND_FACTOR::ONE_MINUS_DST_ALPHA},
            {"constant_color"sv, graphics::BLEND_FACTOR::CONSTANT_COLOR},
            {"one_minus_constant_color"sv, graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR},
            {"constant_alpha"sv, graphics::BLEND_FACTOR::CONSTANT_ALPHA},
            {"one_minus_constant_alpha"sv, graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA},
            {"src_alpha_saturate"sv, graphics::BLEND_FACTOR::SRC_ALPHA_SATURATE},
            {"src1_color"sv, graphics::BLEND_FACTOR::SRC1_COLOR},
            {"one_minus_src1_color"sv, graphics::BLEND_FACTOR::ONE_MINUS_SRC1_COLOR},
            {"src1_alpha"sv, graphics::BLEND_FACTOR::SRC1_ALPHA},
            {"one_minus_src1_alpha"sv, graphics::BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA}
        };

        if (auto it = factors.find(name); it != std::end(factors))
            return it->second;

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
        static const std::unordered_map<std::string_view, graphics::COLOR_COMPONENT> components{
            {"red"sv, graphics::COLOR_COMPONENT::R},
            {"green"sv, graphics::COLOR_COMPONENT::G},
            {"blue"sv, graphics::COLOR_COMPONENT::B},
            {"alpha"sv, graphics::COLOR_COMPONENT::A},
            {"rgb"sv, graphics::COLOR_COMPONENT::RGB},
            {"rgba"sv, graphics::COLOR_COMPONENT::RGBA}
        };

        if (auto it = components.find(name); it != std::end(components))
            return it->second;

        return { };
    }

    std::optional<material_description::specialization_constant> specialization_constant_value(std::string_view type, float value)
    {
        if (type == "float"sv)
            return value;

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