#pragma once

#include <array>
#include <vector>

#include "graphics.hxx"


namespace graphics
{
    enum class CULL_MODE {
        NONE, FRONT, BACK, FRONT_AND_BACK = FRONT | BACK
    };

    enum class POLYGON_FRONT_FACE {
        COUNTER_CLOCKWISE, CLOCKWISE
    };

    enum class POLYGON_MODE {
        FILL, LINE, POINT
    };

    enum class COMPARE_OPERATION {
        NEVER,
        LESS,
        EQUAL,
        LESS_OR_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_OR_EQUAL,
        ALWAYS
    };

    enum class BLEND_STATE_OPERATION {
        CLEAR,
        AND,
        AND_REVERSE,
        COPY,
        AND_INVERTED,
        NO_OP,
        XOR,
        OR,
        NOR,
        EQUIVALENT,
        INVERT,
        OR_REVERSE,
        COPY_INVERTED,
        OR_INVERTED,
        NAND,
        SET
    };

    enum class BLEND_FACTOR {
        ZERO,
        ONE,
        SRC_COLOR,
        ONE_MINUS_SRC_COLOR,
        DST_COLOR,
        ONE_MINUS_DST_COLOR,
        SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA,
        DST_ALPHA,
        ONE_MINUS_DST_ALPHA,
        CONSTANT_COLOR,
        ONE_MINUS_CONSTANT_COLOR,
        CONSTANT_ALPHA,
        ONE_MINUS_CONSTANT_ALPHA,
        SRC_ALPHA_SATURATE,
        SRC1_COLOR,
        ONE_MINUS_SRC1_COLOR,
        SRC1_ALPHA,
        ONE_MINUS_SRC1_ALPHA
    };

    enum class BLEND_OPERATION {
        ADD,
        SUBTRACT
    };

    enum class COLOR_COMPONENT {
        R = 0x01,
        G = 0x02,
        B = 0x04,
        A = 0x08,

        RGB = R | G | B,
        RGBA = R | G | B | A
    };
}

namespace graphics
{
    struct rasterization_state final {
        CULL_MODE cull_mode{CULL_MODE::BACK};
        POLYGON_FRONT_FACE front_face{POLYGON_FRONT_FACE::COUNTER_CLOCKWISE};
        POLYGON_MODE polygon_mode{POLYGON_MODE::FILL};

        float line_width{1.f};

        template<class T, typename std::enable_if_t<std::is_same_v<rasterization_state, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return cull_mode == rhs.cull_mode &&
                front_face == rhs.front_face &&
                polygon_mode == rhs.polygon_mode &&
                line_width == rhs.line_width;
        }
    };

    struct depth_stencil_state final {
        bool depth_test_enable{true};
        bool depth_write_enable{true};

        COMPARE_OPERATION depth_compare_operation{COMPARE_OPERATION::GREATER};

        bool stencil_test_enable{false};

        template<class T, typename std::enable_if_t<std::is_same_v<depth_stencil_state, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return depth_test_enable == rhs.depth_test_enable &&
                depth_write_enable == rhs.depth_write_enable &&
                depth_compare_operation == rhs.depth_compare_operation &&
                stencil_test_enable == rhs.stencil_test_enable;
        }
    };

    struct color_blend_attachment_state final {
        bool blend_enable{false};

        BLEND_FACTOR src_color_blend_factor{BLEND_FACTOR::ONE};
        BLEND_FACTOR dst_color_blend_factor{BLEND_FACTOR::ZERO};

        BLEND_OPERATION color_blend_operation{BLEND_OPERATION::ADD};

        BLEND_FACTOR src_alpha_blend_factor{BLEND_FACTOR::ONE};
        BLEND_FACTOR dst_alpha_blend_factor{BLEND_FACTOR::ZERO};

        BLEND_OPERATION alpha_blend_operation{BLEND_OPERATION::ADD};

        COLOR_COMPONENT color_write_mask{COLOR_COMPONENT::RGBA};

        template<class T, typename std::enable_if_t<std::is_same_v<color_blend_attachment_state, std::decay_t<T>>>* = nullptr>
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

        BLEND_STATE_OPERATION logic_operation{BLEND_STATE_OPERATION::COPY};

        std::array<float, 4> blend_constants{0, 0, 0, 0};

        std::vector<color_blend_attachment_state> attachments;

        template<class T, typename std::enable_if_t<std::is_same_v<color_blend_state, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return logic_operation_enable == rhs.logic_operation_enable &&
                logic_operation == rhs.logic_operation &&
                blend_constants == rhs.blend_constants &&
                attachments == rhs.attachments;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::rasterization_state> {
        std::size_t operator() (graphics::rasterization_state const &state) const noexcept
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
    struct hash<graphics::depth_stencil_state> {
        std::size_t operator() (graphics::depth_stencil_state const &state) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.depth_test_enable);
            boost::hash_combine(seed, state.depth_write_enable);
            boost::hash_combine(seed, state.depth_compare_operation);
            boost::hash_combine(seed, state.stencil_test_enable);

            return seed;
        }
    };

    template<>
    struct hash<graphics::color_blend_attachment_state> {
        std::size_t operator() (graphics::color_blend_attachment_state const &state) const noexcept
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
        std::size_t operator() (graphics::color_blend_state const &state) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, state.logic_operation_enable);
            boost::hash_combine(seed, state.logic_operation);
            boost::hash_combine(seed, state.blend_constants);

            graphics::hash<graphics::color_blend_attachment_state> constexpr color_blend_attachment_state_hasher;

            for (auto &&attachment : state.attachments)
                boost::hash_combine(seed, color_blend_attachment_state_hasher(attachment));

            return seed;
        }
    };
}
