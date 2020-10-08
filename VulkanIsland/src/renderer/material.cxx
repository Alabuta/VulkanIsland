#include <optional>
#include <algorithm>
//#include <ranges>

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
            auto vertex_attributes = indices | ranges::views::transform(transform_indices);
            auto intersection = ranges::views::set_intersection(required_vertex_attributes, vertex_attributes, compare_attributes);

            return ranges::distance(intersection) == ranges::distance(indices);
        };

        auto vertex_layouts = technique.vertex_layouts | ranges::views::filter(filter_vertex_layouts);

        if (ranges::distance(vertex_layouts) == 0)
            return { };

        graphics::vertex_layout vertex_layout;

        auto &&vertex_attributes = vertex_layout.attributes;

        for (auto [semantic, format] : ranges::front(vertex_layouts) | ranges::views::transform(transform_indices))
            vertex_attributes.push_back({semantic, format});

        std::sort(std::begin(vertex_attributes), std::end(vertex_attributes));

        vertex_layout.size_in_bytes = std::accumulate(std::begin(vertex_attributes), std::end(vertex_attributes), vertex_layout.size_in_bytes,
                                                      [] (auto size_in_bytes, auto attribute)
        {
            if (auto format_instance = graphics::instantiate_format(attribute.format); format_instance) {
                return size_in_bytes + std::visit([] (auto &&format)
                {
                    return sizeof(std::remove_cvref_t<decltype(format)>);
                }, *format_instance);
            }

            else throw graphics::exception("unsupported format"s);
        });

        return vertex_layout;
    }

    // TODO:: replace 'renderable_vertex_layout' method argument by reference to renderable instance.
    [[nodiscard]] std::string
    compile_name(std::string_view name, std::uint32_t technique_index, graphics::vertex_layout const &renderable_vertex_layout)
    {
        auto vertexl_layout_name = graphics::to_string(renderable_vertex_layout);
        auto full_name = fmt::format("{}.{}.{}"s, name, technique_index, vertexl_layout_name);

        boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());

        return boost::uuids::to_string(gen(full_name));
    }
}

namespace graphics
{
    std::shared_ptr<graphics::material>
    material_factory::material(std::string_view name, std::uint32_t technique_index, graphics::vertex_layout const &renderable_vertex_layout)
    {
        auto hashed_name = compile_name(name, technique_index, renderable_vertex_layout);

        if (materials_.contains(hashed_name))
            return materials_.at(hashed_name);

        auto &&description = material_description(name);

        auto &&techniques = description.techniques;
        auto &&technique = techniques.at(technique_index);

        auto &&shader_modules = description.shader_modules;
        auto &&shaders_bundle = technique.shaders_bundle;

        auto &&vertex_attributes = description.vertex_attributes;

        if (auto vertex_layout = compatible_vertex_layout(vertex_attributes, technique, renderable_vertex_layout); vertex_layout) {
            std::vector<graphics::shader_stage> shader_stages;

            std::transform(std::cbegin(shaders_bundle), std::cend(shaders_bundle),
                           std::back_inserter(shader_stages), [&shader_modules, &vertex_layout] (auto shader_bundle)
            {
                std::set<graphics::specialization_constant> constants;

                std::uint32_t id = 0;

                for (auto &&value : shader_bundle.specialization_constants) {
                    std::visit([&id, &constants] (auto value)
                    {
                        constants.emplace(id++, value);
                    }, value);
                }

                auto [shader_semantic, shader_name] = shader_modules.at(shader_bundle.module_index);

                auto const shader_technique_index = static_cast<std::uint32_t>(shader_bundle.technique_index);

                shader_name = compile_name(shader_name, shader_technique_index, *vertex_layout);

                return graphics::shader_stage{
                    shader_name, shader_technique_index, shader_semantic, constants
                };
            });

            auto material = std::make_shared<graphics::material>(std::move(shader_stages), *vertex_layout);

            materials_.emplace(hashed_name, material);

            return material;
        }

        else return { };
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
