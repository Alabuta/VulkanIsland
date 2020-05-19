#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <boost/uuid/name_generator.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "graphics/graphics_api.hxx"

#include "material.hxx"


namespace graphics
{
    std::shared_ptr<graphics::material> material_factory::material(std::string_view name, std::uint32_t technique_index)
    {
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

        graphics::vertex_layout vertex_layout{0, { }};

        std::transform(std::cbegin(technique.vertex_layout), std::cend(technique.vertex_layout),
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
        });

        auto material = std::make_shared<graphics::material>(shader_stages, vertex_layout);

        materials_.emplace(key, material);

        return material;
    }

    loader::material_description const &material_factory::material_description(std::string_view name)
    {
        auto key = std::string{name};

        if (material_descriptions_.count(key) == 0)
            material_descriptions_.emplace(key, loader::load_material_description(name));

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
