#pragma once

#include <memory>
#include <vector>
#include <variant>
#include <string>
#include <string_view>
using namespace std::string_view_literals;

#include <boost/cstdfloat.hpp>

#include "graphics/graphics.hxx"
#include "graphics/vertex.hxx"
#include "graphics/pipeline_states.hxx"


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

            graphics::FORMAT format;
        };

        std::vector<vertex_attribute> vertex_attributes;

        using specialization_constant = std::variant<std::int32_t, boost::float32_t>;

        struct shader_bundle final {
            std::size_t module_index;
            std::size_t technique_index;

            std::vector<specialization_constant> specialization_constants;
        };

        using vertex_layout = std::vector<std::size_t>;

        struct technique final {
            std::vector<shader_bundle> shaders_bundle;
            std::vector<vertex_layout> vertex_layouts;
        };

        std::vector<technique> techniques;
    };

    [[nodiscard]] loader::material_description load_material_description(std::string_view name);
}
