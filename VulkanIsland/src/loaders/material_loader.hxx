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
            graphics::PIPELINE_SHADER_STAGE stage;
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

        struct rasterization_state final {
            graphics::CULL_MODE cull_mode{graphics::CULL_MODE::BACK};
            graphics::POLYGON_FRONT_FACE front_face{graphics::POLYGON_FRONT_FACE::COUNTER_CLOCKWISE};
            graphics::POLYGON_MODE polygon_mode{graphics::POLYGON_MODE::FILL};

            float line_width{1.f};
        };

        std::vector<rasterization_state> rasterization_states;

        struct depth_stencil_state final {
            bool depth_test_enable{true};
            bool depth_write_enable{true};

            graphics::COMPARE_OPERATION depth_compare_operation{graphics::COMPARE_OPERATION::GREATER};

            bool stencil_test_enable{false};
        };

        std::vector<depth_stencil_state> depth_stencil_states;

        struct color_blend_state final {
            bool logic_operation_enable{false};

            graphics::BLEND_STATE_OPERATION logic_operation{graphics::BLEND_STATE_OPERATION::COPY};

            std::array<float, 4> blend_constants{0, 0, 0, 0};

            std::vector<std::size_t> attachments;
        };

        std::vector<color_blend_state> color_blend_states;

        struct color_blend_attachment_state final {
            bool blend_enable{false};

            graphics::BLEND_FACTOR src_color_blend_factor{graphics::BLEND_FACTOR::ONE};
            graphics::BLEND_FACTOR dst_color_blend_factor{graphics::BLEND_FACTOR::ZERO};

            graphics::BLEND_OPERATION color_blend_operation{graphics::BLEND_OPERATION::ADD};

            graphics::BLEND_FACTOR src_alpha_blend_factor{graphics::BLEND_FACTOR::ONE};
            graphics::BLEND_FACTOR dst_alpha_blend_factor{graphics::BLEND_FACTOR::ZERO};

            graphics::BLEND_OPERATION alpha_blend_operation{graphics::BLEND_OPERATION::ADD};

            graphics::COLOR_COMPONENT color_write_mask{graphics::COLOR_COMPONENT::RGBA};
        };

        std::vector<color_blend_attachment_state> color_blend_attachment_states;
    };

    [[nodiscard]] loader::material_description load_material_description(std::string_view name);
}
