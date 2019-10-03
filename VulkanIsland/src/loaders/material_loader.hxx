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

            bool normalized{false};
        };

        std::vector<vertex_attribute> vertex_attributes;

        struct shader_bundle final {
            std::size_t module_index;
            std::size_t technique_index;
        };

        struct technique final {
            std::vector<shader_bundle> shaders_bundle;
            std::vector<std::size_t> vertex_layout;
        };

        std::vector<technique> techniques;
    };

    [[nodiscard]] loader::material_description load_material_description(std::string_view name);
}
