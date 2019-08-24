#pragma once

#include <array>
#include <vector>

#include "graphics.hxx"

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

struct ColorBlendAttachmentState final {
    bool blendEnable{false};

    BLEND_FACTOR srcColorBlendFactor{BLEND_FACTOR::ONE};
    BLEND_FACTOR dstColorBlendFactor{BLEND_FACTOR::ZERO};

    BLEND_OPERATION colorBlendOperation{BLEND_OPERATION::ADD};

    BLEND_FACTOR srcAlphaBlendFactor{BLEND_FACTOR::ONE};
    BLEND_FACTOR dstAlphaBlendFactor{BLEND_FACTOR::ZERO};

    BLEND_OPERATION alphaBlendOperation{BLEND_OPERATION::ADD};

    COLOR_COMPONENT colorWriteMask{COLOR_COMPONENT::RGBA};
};

struct ColorBlendState final {
    bool logicOperationEnable{false};

    BLEND_STATE_OPERATION logicOperation{BLEND_STATE_OPERATION::COPY};

    std::array<float, 4> blendConstants{0, 0, 0, 0};

    std::vector<ColorBlendAttachmentState> attachments;
};



namespace graphics
{
    struct render_pass_attachment final {
        ;
    };

    struct attachment_description final {
        graphics::FORMAT view_format;
        std::uint32_t samples_count{1};

        graphics::ATTACHMENT_LOAD_TREATMENT load_op;
        graphics::ATTACHMENT_STORE_TREATMENT store_op;

        // At the begining of a render pass.
        graphics::IMAGE_LAYOUT initial_layout;
        // At the end of a render pass.
        graphics::IMAGE_LAYOUT final_layout;
    };

    struct attachment_reference final {
        std::uint32_t index;

        // During a subpass.
        graphics::IMAGE_LAYOUT layout;

        template<class T, typename std::enable_if_t<std::is_same_v<attachment_reference, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return index == rhs.index && layout == rhs.layout;
        }

        template<class T, typename std::enable_if_t<std::is_same_v<attachment_reference, std::decay_t<T>>>* = nullptr>
        auto constexpr operator< (T &&rhs) const
        {
            return index < rhs.index;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::attachment_reference> {
        std::size_t operator() (graphics::attachment_reference const &attachment_reference) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, attachment_reference.index);
            boost::hash_combine(seed, attachment_reference.layout);

            return seed;
        }
    };
}
