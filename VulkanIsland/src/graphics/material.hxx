#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "vulkan/device.hxx"

#include "graphics.hxx"
#include "vertex.hxx"
#include "loaders/material_loader.hxx"
#include "shader_program.hxx"


namespace graphics
{
    struct material final {

        material(std::vector<graphics::shader_stage> shader_stages, graphics::vertex_layout vertex_layout)
            : shader_stages{shader_stages}, vertex_layout{vertex_layout}  { }

        std::vector<graphics::shader_stage> shader_stages;

        graphics::vertex_layout vertex_layout;

        // TODO:: descriptor set and pipeline layout.
    };
}

namespace graphics
{
    class material_factory final {
    public:

        [[nodiscard]] std::shared_ptr<graphics::material> material(std::string_view name, std::uint32_t technique_index);

    private:

        std::map<std::pair<std::string, std::uint32_t>, std::shared_ptr<graphics::material>> materials_;

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
