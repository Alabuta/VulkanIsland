#include <optional>
#include <algorithm>
#include <ranges>

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <range/v3/all.hpp>

#include <fmt/format.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "graphics/graphics_api.hxx"

#include "material.hxx"


namespace
{
    std::optional<graphics::vertex_layout>
    compatible_vertex_layout(std::vector<loader::material_description::vertex_attribute> const &material_vertex_attributes,
                             loader::material_description::technique const &technique, graphics::vertex_layout const &renderable_vertex_layout)
    {
        auto &&required_vertex_attributes = renderable_vertex_layout.attributes;

        auto compare_attributes = [&] (auto &&lhs, auto &&rhs)
        {
            if (lhs.semantic != rhs.semantic)
                return true;

            if (lhs.format != rhs.format)
                return true;

            return false;
        };

        auto transform_indices = [&] (auto index) { return material_vertex_attributes.at(index); };

        auto filter_vertex_layouts = [&] (auto &&indices)
        {
            auto vertex_attributes = indices | std::views::transform(transform_indices);
            auto intersection = ranges::views::set_intersection(required_vertex_attributes, vertex_attributes, compare_attributes);

            return std::ranges::distance(intersection) == std::ranges::distance(indices);
        };

        auto filtered_vertex_layouts = technique.vertex_layouts | std::views::filter(filter_vertex_layouts);

        if (std::ranges::distance(filtered_vertex_layouts) == 0)
            return { };

        auto filtered_vertex_attributes = filtered_vertex_layouts.front() | std::views::transform(transform_indices);

        graphics::vertex_layout vertex_layout;
        auto &&vertex_attributes = vertex_layout.attributes;

        for (auto [semantic, format] : filtered_vertex_attributes)
            vertex_attributes.push_back({semantic, format});

        std::sort(std::begin(vertex_attributes), std::end(vertex_attributes));

        vertex_layout.size_bytes = std::accumulate(std::begin(vertex_attributes), std::end(vertex_attributes), vertex_layout.size_bytes,
                                                   [] (auto total_size_bytes, auto attribute)
        {
            auto size_bytes = graphics::size_bytes(attribute.format);

            if (size_bytes == 0)
                throw graphics::exception("unsupported format");

            return total_size_bytes + size_bytes;
        });

        return vertex_layout;
    }

    // TODO:: replace 'renderable_vertex_layout' method argument by reference to renderable instance.
    [[nodiscard]] std::string
    compile_name(std::string_view name, std::uint32_t technique_index, graphics::vertex_layout const &renderable_vertex_layout, graphics::PRIMITIVE_TOPOLOGY primitive_topology)
    {
        auto vertex_layout_name = graphics::to_string(renderable_vertex_layout);
        auto primitive_input_name = graphics::to_string(primitive_topology);

        auto full_name = fmt::format("{}.{}.{}.{}", name, technique_index, vertex_layout_name, primitive_input_name);

        const boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());

        return boost::uuids::to_string(gen(full_name));
    }

    [[nodiscard]]
    std::string compile_name(std::string_view name, graphics::SHADER_STAGE stage, std::uint32_t technique_index, graphics::vertex_layout const &renderable_vertex_layout, graphics::PRIMITIVE_TOPOLOGY primitive_topology)
    {
        auto vertex_layout_name = graphics::to_string(renderable_vertex_layout);
        auto primitive_input_name = graphics::to_string(primitive_topology);

        std::string full_name;

        switch (stage)
        {
            case graphics::SHADER_STAGE::VERTEX:
                full_name = fmt::format("{}.{}.{}", name, technique_index, vertex_layout_name);
                break;

            case graphics::SHADER_STAGE::GEOMETRY:
                full_name = fmt::format("{}.{}.{}", name, technique_index, primitive_input_name);
                break;

            case graphics::SHADER_STAGE::FRAGMENT:
            case graphics::SHADER_STAGE::TESS_CONTROL:
            case graphics::SHADER_STAGE::TESS_EVAL:
            case graphics::SHADER_STAGE::COMPUTE:
            case graphics::SHADER_STAGE::ALL_GRAPHICS_SHADER_STAGES:
            case graphics::SHADER_STAGE::ALL_SHADER_STAGES:
                full_name = fmt::format("{}.{}", name, technique_index);
                break;

            default:
                break;
        }

        const boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());
        return boost::uuids::to_string(gen(full_name));
    }
}

namespace graphics
{
    std::shared_ptr<graphics::material>
    material_factory::material(std::string_view material_name, std::uint32_t technique_index, graphics::vertex_layout const &renderable_vertex_layout, graphics::PRIMITIVE_TOPOLOGY primitive_topology)
    {
        auto &&description = material_description(material_name);

        auto &&techniques = description.techniques;
        auto &&technique = techniques.at(technique_index);

        auto &&shader_modules = description.shader_modules;
        auto &&shaders_bundle = technique.shaders_bundle;

        auto &&vertex_attributes = description.vertex_attributes;

        auto vertex_layout = compatible_vertex_layout(vertex_attributes, technique, renderable_vertex_layout);
        if (!vertex_layout)
            throw graphics::exception("failed to create compatible vertex layout");

        auto hashed_name = compile_name(material_name, technique_index, *vertex_layout, primitive_topology);

        if (materials_.contains(hashed_name))
            return materials_.at(hashed_name);

        std::vector<graphics::shader_stage> shader_stages;

        std::ranges::transform(shaders_bundle, std::back_inserter(shader_stages), [&shader_modules, &vertex_layout, primitive_topology] (auto shader_bundle)
        {
            std::set<graphics::specialization_constant> constants;

            for (std::uint32_t id = 0; auto &&constant : shader_bundle.specialization_constants) {
                std::visit([&id, &constants] (auto value)
                {
                    constants.emplace(id++, value);
                }, constant);
            }

            auto [shader_semantic, shader_name] = shader_modules.at(shader_bundle.module_index);

            auto const shader_technique_index = static_cast<std::uint32_t>(shader_bundle.technique_index);

            shader_name = compile_name(shader_name, shader_semantic, shader_technique_index, *vertex_layout, primitive_topology);

            return graphics::shader_stage{
                shader_name, shader_technique_index, shader_semantic, constants
            };
        });

        auto material = std::make_shared<graphics::material>(std::move(shader_stages), *vertex_layout, primitive_topology);

        materials_.emplace(hashed_name, material);

        return material;
    }

    loader::material_description const &material_factory::material_description(std::string_view name)
    {
        auto key = std::string{name};

        if (!material_descriptions_.contains(key)) {
            auto material_description = loader::load_material_description(name);
            auto &&vertex_attributes = material_description.vertex_attributes;

            for (auto &&technique : material_description.techniques) {
                for (auto &&indices : technique.vertex_layouts) {
                    ranges::sort(indices, [&] (auto lhs, auto rhs)
                    {
                        return vertex_attributes.at(lhs).semantic < vertex_attributes.at(rhs).semantic;
                    });
                }
            }

            material_descriptions_.emplace(key, std::move(material_description));
        }

        return material_descriptions_.at(key);
    }
}

namespace graphics
{
    std::size_t hash<graphics::material>::operator() (graphics::material const &material) const
    {
        std::size_t seed = 0;

        graphics::hash<graphics::shader_stage> constexpr shader_stage_hasher;

        for (auto &&shader_stage : material.shader_stages)
            boost::hash_combine(seed, shader_stage_hasher(shader_stage));

        return seed;
    }
}
