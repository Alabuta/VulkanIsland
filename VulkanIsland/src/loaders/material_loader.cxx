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
            {"r8i_norm"sv, graphics::FORMAT::R8_SNORM},
            {"rg8i_norm"sv, graphics::FORMAT::RG8_SNORM},
            {"rgb8i_norm"sv, graphics::FORMAT::RGB8_SNORM},
            {"rgba8i_norm"sv, graphics::FORMAT::RGBA8_SNORM},

            {"r8ui_norm"sv, graphics::FORMAT::R8_UNORM},
            {"rg8ui_norm"sv, graphics::FORMAT::RG8_UNORM},
            {"rgb8ui_norm"sv, graphics::FORMAT::RGB8_UNORM},
            {"rgba8ui_norm"sv, graphics::FORMAT::RGBA8_UNORM},

            {"r16i_norm"sv, graphics::FORMAT::R16_SNORM},
            {"rg16i_norm"sv, graphics::FORMAT::RG16_SNORM},
            {"rgb16i_norm"sv, graphics::FORMAT::RGB16_SNORM},
            {"rgba16i_norm"sv, graphics::FORMAT::RGBA16_SNORM},

            {"r16ui_norm"sv, graphics::FORMAT::R16_UNORM},
            {"rg16ui_norm"sv, graphics::FORMAT::RG16_UNORM},
            {"rgb16ui_norm"sv, graphics::FORMAT::RGB16_UNORM},
            {"rgba16ui_norm"sv, graphics::FORMAT::RGBA16_UNORM},

            {"r8i_scaled"sv, graphics::FORMAT::R8_SSCALED},
            {"rg8i_scaled"sv, graphics::FORMAT::RG8_SSCALED},
            {"rgb8i_scaled"sv, graphics::FORMAT::RGB8_SSCALED},
            {"rgba8i_scaled"sv, graphics::FORMAT::RGBA8_SSCALED},

            {"r8ui_scaled"sv, graphics::FORMAT::R8_USCALED},
            {"rg8ui_scaled"sv, graphics::FORMAT::RG8_USCALED},
            {"rgb8ui_scaled"sv, graphics::FORMAT::RGB8_USCALED},
            {"rgba8ui_scaled"sv, graphics::FORMAT::RGBA8_USCALED},

            {"r16i_scaled"sv, graphics::FORMAT::R16_SSCALED},
            {"rg16i_scaled"sv, graphics::FORMAT::RG16_SSCALED},
            {"rgb16i_scaled"sv, graphics::FORMAT::RGB16_SSCALED},
            {"rgba16i_scaled"sv, graphics::FORMAT::RGBA16_SSCALED},

            {"r16ui_scaled"sv, graphics::FORMAT::R16_USCALED},
            {"rg16ui_scaled"sv, graphics::FORMAT::RG16_USCALED},
            {"rgb16ui_scaled"sv, graphics::FORMAT::RGB16_USCALED},
            {"rgba16ui_scaled"sv, graphics::FORMAT::RGBA16_USCALED},

            {"r8i"sv, graphics::FORMAT::R8_SINT},
            {"rg8i"sv, graphics::FORMAT::RG8_SINT},
            {"rgb8i"sv, graphics::FORMAT::RGB8_SINT},
            {"rgba8i"sv, graphics::FORMAT::RGBA8_SINT},

            {"r8ui"sv, graphics::FORMAT::R8_UINT},
            {"rg8ui"sv, graphics::FORMAT::RG8_UINT},
            {"rgb8ui"sv, graphics::FORMAT::RGB8_UINT},
            {"rgba8ui"sv, graphics::FORMAT::RGBA8_UINT},

            {"r16i"sv, graphics::FORMAT::R16_SINT},
            {"rg16i"sv, graphics::FORMAT::RG16_SINT},
            {"rgb16i"sv, graphics::FORMAT::RGB16_SINT},
            {"rgba16i"sv, graphics::FORMAT::RGBA16_SINT},

            {"r16ui"sv, graphics::FORMAT::R16_UINT},
            {"rg16ui"sv, graphics::FORMAT::RG16_UINT},
            {"rgb16ui"sv, graphics::FORMAT::RGB16_UINT},
            {"rgba16ui"sv, graphics::FORMAT::RGBA16_UINT},

            {"r32i"sv, graphics::FORMAT::R32_SINT},
            {"rg32i"sv, graphics::FORMAT::RG32_SINT},
            {"rgb32i"sv, graphics::FORMAT::RGB32_SINT},
            {"rgba32i"sv, graphics::FORMAT::RGBA32_SINT},

            {"r32ui"sv, graphics::FORMAT::R32_UINT},
            {"rg32ui"sv, graphics::FORMAT::RG32_UINT},
            {"rgb32ui"sv, graphics::FORMAT::RGB32_UINT},
            {"rgba32ui"sv, graphics::FORMAT::RGBA32_UINT},

            {"r32f"sv, graphics::FORMAT::R32_SFLOAT},
            {"rg32f"sv, graphics::FORMAT::RG32_SFLOAT},
            {"rgb32f"sv, graphics::FORMAT::RGB32_SFLOAT},
            {"rgba32f"sv, graphics::FORMAT::RGBA32_SFLOAT},

            {"r64f"sv, graphics::FORMAT::R64_SFLOAT},
            {"rg64f"sv, graphics::FORMAT::RG64_SFLOAT},
            {"rgb64f"sv, graphics::FORMAT::RGB64_SFLOAT},
            {"rgba64f"sv, graphics::FORMAT::RGBA64_SFLOAT}
        };

        if (auto it = formats.find(type); it != std::end(formats))
            return it->second;

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

        if (auto format = loader::attribute_format(j.at("type"s).get<std::string>()); format)
            vertex_attribute.format = *format;

        else throw std::runtime_error("unsupported vertex attribute format"s);
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