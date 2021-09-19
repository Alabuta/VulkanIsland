#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "vulkan/device.hxx"

#include "graphics/graphics.hxx"
#include "graphics/vertex.hxx"
#include "graphics/shader_program.hxx"
#include "loaders/material_loader.hxx"


namespace graphics
{
    struct material final {
        material(std::vector<graphics::shader_stage> shader_stages, graphics::vertex_layout vertex_layout, graphics::PRIMITIVE_TOPOLOGY topology)
            : shader_stages{shader_stages}, vertex_layout{vertex_layout}, topology{topology} { }

        std::vector<graphics::shader_stage> shader_stages;

        graphics::vertex_layout vertex_layout;
        graphics::PRIMITIVE_TOPOLOGY topology;

        // TODO:: descriptor set and pipeline layout.
    };
}

namespace graphics
{
    class material_factory final {
    public:

        // TODO:: replace 'renderable_vertex_layout' method argument by reference to renderable instance.
        [[nodiscard]] std::shared_ptr<graphics::material>
        material(std::string_view name, std::uint32_t technique, graphics::vertex_layout const &renderable_vertex_layout, graphics::PRIMITIVE_TOPOLOGY topology);

    private:

        std::map<std::string, std::shared_ptr<graphics::material>> materials_;

        // TODO:: move to general loader manager.
        std::unordered_map<std::string, loader::material_description> material_descriptions_;

        [[nodiscard]] loader::material_description const &material_description(std::string_view name);
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::material> {
        std::size_t operator() (graphics::material const &material) const;
    };
}
