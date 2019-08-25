#pragma once

#include <memory>
#include <vector>

#include <string>
#include <string_view>
using namespace std::string_view_literals;

#include "renderer/graphics.hxx"
#include "renderer/vertex.hxx"
#include "renderer/pipeline_states.hxx"


namespace loader
{
    struct material_description final {
        std::string name;

        struct shader_module final {
            graphics::SHADER_STAGE stage;
            std::string name;
        };

        std::vector<shader_module> shader_modules;

        struct vertex_attribute final {
            vertex::attribute_semantic semantic;
            vertex::attribute_type type;
        };

        std::vector<vertex_attribute> vertex_attributes;

        struct shader_bundle final {
            std::size_t module_index;
            std::size_t technique_index;
        };

        struct technique final {
            std::vector<shader_bundle> shaders_bundle;
            std::vector<std::size_t> vertex_layout;

            std::size_t rasterization_state;
            std::size_t depth_stencil_state;
            std::size_t color_blend_state;
        };

        std::vector<technique> techniques;

        std::vector<graphics::rasterization_state> rasterization_states;
        std::vector<graphics::depth_stencil_state> depth_stencil_states;

        std::vector<graphics::color_blend_state> color_blend_states;
        std::vector<graphics::color_blend_attachment_state> color_blend_attachment_states;
    };

    [[nodiscard]] loader::material_description load_material_description(std::string_view name);
}
