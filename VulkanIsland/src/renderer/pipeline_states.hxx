#pragma once

#include <array>
#include <vector>

#include "utility/mpl.hxx"
#include "graphics.hxx"
#include "vertex.hxx"


namespace graphics
{
    struct vertex_input_state final {
        std::vector<graphics::vertex_input_binding> binding_descriptions;
        std::vector<graphics::vertex_input_attribute> attribute_descriptions;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, vertex_input_state>
        auto constexpr operator== (T &&rhs) const
        {
            return binding_descriptions == rhs.binding_descriptions &&
                attribute_descriptions == rhs.attribute_descriptions;
        }
    };

    struct rasterization_state final {
        graphics::CULL_MODE cull_mode{graphics::CULL_MODE::BACK};
        graphics::POLYGON_FRONT_FACE front_face{graphics::POLYGON_FRONT_FACE::COUNTER_CLOCKWISE};
        graphics::POLYGON_MODE polygon_mode{graphics::POLYGON_MODE::FILL};

        float line_width{1.f};

        template<class T> requires std::same_as<std::remove_cvref_t<T>, rasterization_state>
        auto constexpr operator== (T &&rhs) const
        {
            return cull_mode == rhs.cull_mode &&
                front_face == rhs.front_face &&
                polygon_mode == rhs.polygon_mode &&
                line_width == rhs.line_width;
        }
    };

    struct stencil_state final {
        graphics::STENCIL_OPERATION fail{graphics::STENCIL_OPERATION::KEEP};
        graphics::STENCIL_OPERATION pass{graphics::STENCIL_OPERATION::KEEP};
        graphics::STENCIL_OPERATION depth_fail{graphics::STENCIL_OPERATION::KEEP};

        graphics::COMPARE_OPERATION compare_operation{graphics::COMPARE_OPERATION::GREATER};

        std::uint32_t compare_mask{0};
        std::uint32_t write_mask{0};
        std::uint32_t reference{0};

        template<class T> requires std::same_as<std::remove_cvref_t<T>, stencil_state>
        auto constexpr operator== (T &&rhs) const
        {
            return fail == rhs.fail &&
                pass == rhs.pass &&
                depth_fail == rhs.depthFail &&
                compare_operation == rhs.compare_operation &&
                compare_mask == rhs.compare_mask &&
                write_mask == rhs.write_mask &&
                reference == rhs.reference;
        }
    };

    struct depth_stencil_state final {
        bool depth_test_enable{true};
        bool depth_write_enable{true};

        graphics::COMPARE_OPERATION depth_compare_operation{graphics::COMPARE_OPERATION::GREATER};

        bool depth_bounds_test_enable{false};
        std::array<float, 2> depth_bounds{0, 1};

        bool stencil_test_enable{false};

        graphics::stencil_state front_stencil_state;
        graphics::stencil_state back_stencil_state;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, depth_stencil_state>
        auto constexpr operator== (T &&rhs) const
        {
            return depth_test_enable == rhs.depth_test_enable &&
                depth_write_enable == rhs.depth_write_enable &&
                depth_compare_operation == rhs.depth_compare_operation &&
                depth_bounds_test_enable == rhs.depth_bounds_test_enable &&
                depth_bounds == rhs.depth_bounds &&
                stencil_test_enable == rhs.stencil_test_enable &&
                front_stencil_state == rhs.front_stencil_state &&
                back_stencil_state == rhs.back_stencil_state;
        }
    };

    struct color_blend_attachment_state final {
        bool blend_enable{false};

        graphics::BLEND_FACTOR src_color_blend_factor{graphics::BLEND_FACTOR::ONE};
        graphics::BLEND_FACTOR dst_color_blend_factor{graphics::BLEND_FACTOR::ZERO};

        graphics::BLEND_OPERATION color_blend_operation{graphics::BLEND_OPERATION::ADD};

        graphics::BLEND_FACTOR src_alpha_blend_factor{graphics::BLEND_FACTOR::ONE};
        graphics::BLEND_FACTOR dst_alpha_blend_factor{graphics::BLEND_FACTOR::ZERO};

        graphics::BLEND_OPERATION alpha_blend_operation{graphics::BLEND_OPERATION::ADD};

        graphics::COLOR_COMPONENT color_write_mask{graphics::COLOR_COMPONENT::RGBA};

        template<class T> requires std::same_as<std::remove_cvref_t<T>, color_blend_attachment_state>
        auto constexpr operator== (T &&rhs) const
        {
            return blend_enable == rhs.blend_enable &&
                src_color_blend_factor == rhs.src_color_blend_factor &&
                dst_color_blend_factor == rhs.dst_color_blend_factor &&
                color_blend_operation == rhs.color_blend_operation &&
                src_alpha_blend_factor == rhs.src_alpha_blend_factor &&
                dst_alpha_blend_factor == rhs.dst_alpha_blend_factor &&
                alpha_blend_operation == rhs.alpha_blend_operation &&
                color_write_mask == rhs.color_write_mask;
        }
    };

    struct color_blend_state final {
        bool logic_operation_enable{false};

        graphics::BLEND_STATE_OPERATION logic_operation{graphics::BLEND_STATE_OPERATION::COPY};

        std::array<float, 4> blend_constants{0, 0, 0, 0};

        std::vector<graphics::color_blend_attachment_state> attachment_states;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, color_blend_state>
        auto constexpr operator== (T &&rhs) const
        {
            return logic_operation_enable == rhs.logic_operation_enable &&
                logic_operation == rhs.logic_operation &&
                blend_constants == rhs.blend_constants &&
                attachment_states == rhs.attachments;
        }
    };

    struct pipeline_states final {
        graphics::PRIMITIVE_TOPOLOGY primitive_topology;
        graphics::vertex_input_state vertex_input_state;

        graphics::rasterization_state rasterization_state;
        graphics::depth_stencil_state depth_stencil_state;
        graphics::color_blend_state color_blend_state;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, pipeline_states>
        auto constexpr operator== (T &&rhs) const
        {
            return primitive_topology == rhs.primitive_topology &&
                vertex_input_state == rhs.vertex_input_state &&
                rasterization_state == rhs.rasterization_state &&
                depth_stencil_state == rhs.depth_stencil_state &&
                color_blend_state == rhs.color_blend_state;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::vertex_input_state> {
        std::size_t operator() (graphics::vertex_input_state const &state) const
        {
            std::size_t seed = 0;

            graphics::hash<graphics::vertex_input_binding> constexpr binding_hasher;

            for (auto &&binding_description : state.binding_descriptions)
                boost::hash_combine(seed, binding_hasher(binding_description));

            graphics::hash<graphics::vertex_input_attribute> constexpr attribute_hasher;

            for (auto &&attribute_description : state.attribute_descriptions)
                boost::hash_combine(seed, attribute_hasher(attribute_description));

            return seed;
        }
    };

    template<>
    struct hash<graphics::rasterization_state> {
        std::size_t operator() (graphics::rasterization_state const &state) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.cull_mode);
            boost::hash_combine(seed, state.front_face);
            boost::hash_combine(seed, state.polygon_mode);
            boost::hash_combine(seed, state.line_width);

            return seed;
        }
    };

    template<>
    struct hash<graphics::stencil_state> {
        std::size_t operator() (graphics::stencil_state const &state) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.fail);
            boost::hash_combine(seed, state.pass);
            boost::hash_combine(seed, state.depth_fail);

            boost::hash_combine(seed, state.compare_operation);

            boost::hash_combine(seed, state.compare_mask);
            boost::hash_combine(seed, state.write_mask);
            boost::hash_combine(seed, state.reference);

            return seed;
        }
    };

    template<>
    struct hash<graphics::depth_stencil_state> {
        std::size_t operator() (graphics::depth_stencil_state const &state) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.depth_test_enable);
            boost::hash_combine(seed, state.depth_write_enable);
            boost::hash_combine(seed, state.depth_compare_operation);

            boost::hash_combine(seed, state.depth_bounds_test_enable);
            boost::hash_combine(seed, state.depth_bounds);

            boost::hash_combine(seed, state.stencil_test_enable);

            graphics::hash<graphics::stencil_state> constexpr stencil_state_hasher;

            boost::hash_combine(seed, stencil_state_hasher(state.front_stencil_state));
            boost::hash_combine(seed, stencil_state_hasher(state.back_stencil_state));

            return seed;
        }
    };

    template<>
    struct hash<graphics::color_blend_attachment_state> {
        std::size_t operator() (graphics::color_blend_attachment_state const &state) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.blend_enable);
            boost::hash_combine(seed, state.src_color_blend_factor);
            boost::hash_combine(seed, state.dst_color_blend_factor);
            boost::hash_combine(seed, state.color_blend_operation);
            boost::hash_combine(seed, state.src_alpha_blend_factor);
            boost::hash_combine(seed, state.dst_alpha_blend_factor);
            boost::hash_combine(seed, state.alpha_blend_operation);
            boost::hash_combine(seed, state.color_write_mask);

            return seed;
        }
    };

    template<>
    struct hash<graphics::color_blend_state> {
        std::size_t operator() (graphics::color_blend_state const &state) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.logic_operation_enable);
            boost::hash_combine(seed, state.logic_operation);
            boost::hash_combine(seed, state.blend_constants);

            graphics::hash<graphics::color_blend_attachment_state> constexpr hasher;

            for (auto &&attachment : state.attachment_states)
                boost::hash_combine(seed, hasher(attachment));

            return seed;
        }
    };

    template<>
    struct hash<graphics::pipeline_states> {
        std::size_t operator() (graphics::pipeline_states const &states) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, states.primitive_topology);

            graphics::hash<graphics::vertex_input_state> constexpr vertex_input_state_hasher;
            boost::hash_combine(seed, vertex_input_state_hasher(states.vertex_input_state));

            graphics::hash<graphics::rasterization_state> constexpr rasterization_state_hasher;
            boost::hash_combine(seed, rasterization_state_hasher(states.rasterization_state));

            graphics::hash<graphics::depth_stencil_state> constexpr depth_stencil_state_hasher;
            boost::hash_combine(seed, depth_stencil_state_hasher(states.depth_stencil_state));

            graphics::hash<graphics::color_blend_state> constexpr color_blend_state_hasher;
            boost::hash_combine(seed, color_blend_state_hasher(states.color_blend_state));

            return seed;
        }
    };
}
