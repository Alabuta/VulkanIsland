#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <fmt/format.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <range/v3/all.hpp>

#include "graphics/graphics.hxx"
#include "graphics/graphics_api.hxx"

#include "material.hxx"


namespace
{
    bool compatible(std::vector<loader::material_description::vertex_attribute> const &material_vertex_attributes,
                    loader::material_description::technique const &technique, graphics::vertex_layout const &required_vertex_layout)
    {
        using namespace ranges;

        auto &&required_vertex_attributes = required_vertex_layout.attributes;

        auto pred_0 = [&required_attributes = required_vertex_layout.attributes] (auto &&indices)
        {
            return std::size(required_attributes) >= std::size(indices);
        };

        auto pred_1 = [&] (auto &&lhs, auto &&rhs)
        {
            if (lhs.semantic != rhs.semantic)
                return false;

            if (lhs.format != rhs.format)
                return false;

            return true;
        };

        auto b = [&] (auto index) { return material_vertex_attributes.at(index); };

        auto pred_2 = [&] (auto &&indices)
        {
            if (std::size(required_vertex_layout.attributes) < std::size(indices))
                return false;

            auto vertex_attributes = indices
                                   | ranges::view::transform([&] (auto index) { return material_vertex_attributes.at(index); });

            auto intersection = ranges::view::set_intersection(required_vertex_attributes, vertex_attributes, pred_1);

            return ranges::distance(intersection) == ranges::distance(indices);
        };

        /*for (auto &&indices : technique.vertex_layouts | ranges::views::filter(pred_0)) {
            ;
        }*/

        auto vertex_layouts = technique.vertex_layouts
                            //| ranges::views::filter(pred_0)
                            | ranges::views::filter(pred_2);
                            //| ranges::views::transform(b)
                            //| ranges::to<std::vector<graphics::vertex_layout>>();

        if (ranges::distance(vertex_layouts) == 0)
            return false;

        auto vl = ranges::front(vertex_layouts) | ranges::views::transform(b);// | ranges::to<graphics::vertex_layout>();

        for (auto &&xxx : vl) {
            fmt::print("{}", graphics::to_string(xxx.semantic));
        }

        fmt::print("{}", ranges::distance(vertex_layouts));


        /*for (auto &&vertex_layout_indices : technique.vertex_layouts) {
            graphics::vertex_layout vertex_layout{0, { }};

            auto &offset_in_bytes = vertex_layout.size_in_bytes;

            auto attributes = vertex_layout_indices | ranges::views::transform([&] (auto vertex_layout_index)
            {
                auto [semantic, format] = vertex_attributes.at(vertex_layout_index);

                graphics::vertex_attribute vertex_attribute{semantic, format, offset_in_bytes};

                if (auto fmt = graphics::instantiate_format(format); fmt) {
                    offset_in_bytes += std::visit([] (auto &&format)
                    {
                        return sizeof(std::remove_cvref_t<decltype(format)>);
                    }, *fmt);
                }

                else throw graphics::exception("unsupported format"s);

                return vertex_attribute;

            }) | ranges::to<std::vector<graphics::vertex_attribute>>();

            fmt::print("{}", attributes.size());
        }*/

        return false;
    }
}

namespace graphics
{
    std::shared_ptr<graphics::material>
    material_factory::material(std::string_view name, std::uint32_t technique_index, graphics::vertex_layout const &required_vertex_layout)
    {
        /*auto vertexl_layout_name = graphics::to_string(vl);
        auto full_name = fmt::format("{}.{}.{}"s, name, technique_index, vertexl_layout_name);

        boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());
        auto hashed_name = boost::uuids::to_string(gen(full_name));*/

        auto const key = std::pair{std::string{name}, technique_index};

        if (materials_.contains(key))
            return materials_.at(key);

        auto &&description = material_description(name);

        auto &&techniques = description.techniques;
        auto &&technique = techniques.at(technique_index);

        auto &&shader_modules = description.shader_modules;
        auto &&shaders_bundle = technique.shaders_bundle;

        std::vector<graphics::shader_stage> shader_stages;

        std::transform(std::cbegin(shaders_bundle), std::cend(shaders_bundle),
                       std::back_inserter(shader_stages), [&shader_modules] (auto shader_bundle)
        {
            std::set<graphics::specialization_constant> constants;

            std::uint32_t id = 0;

            for (auto &&value : shader_bundle.specialization_constants) {
                std::visit([&id, &constants] (auto value)
                {
                    constants.emplace(id++, value);
                }, value);
            }

            auto &&[shader_semantic, shader_name] = shader_modules.at(shader_bundle.module_index);

            return graphics::shader_stage{
                shader_name, static_cast<std::uint32_t>(shader_bundle.technique_index), shader_semantic, constants
            };
        });

        auto &&vertex_attributes = description.vertex_attributes;

        compatible(vertex_attributes, technique, required_vertex_layout);


        /*std::transform(std::cbegin(technique.vertex_layout), std::cend(technique.vertex_layout),
                       std::back_inserter(vertex_layout.attributes),
                       [vertex_attributes, &offset_in_bytes = vertex_layout.size_in_bytes] (auto vertex_layout_index) mutable
        {
            auto [semantic, format] = vertex_attributes.at(vertex_layout_index);

            graphics::vertex_attribute const vertex_attribute{semantic, format, offset_in_bytes};

            if (auto fmt = graphics::instantiate_format(format); fmt) {
                offset_in_bytes += std::visit([] (auto &&format)
                {
                    return sizeof(std::remove_cvref_t<decltype(format)>);
                }, *fmt);
            }

            else throw graphics::exception("unsupported format"s);

            return vertex_attribute;
        });*/

        /*auto material = std::make_shared<graphics::material>(shader_stages, vertex_layout);

        materials_.emplace(key, material);

        return material;*/
        return { };
    }

    loader::material_description const &material_factory::material_description(std::string_view name)
    {
        auto key = std::string{name};

        if (material_descriptions_.count(key) == 0) {
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
