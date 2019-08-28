#pragma once

#include <array>
#include <vector>
#include <variant>

#include "graphics.hxx"

struct ColorBlendAttachmentState final {
    bool blendEnable{false};

    graphics::BLEND_FACTOR srcColorBlendFactor{graphics::BLEND_FACTOR::ONE};
    graphics::BLEND_FACTOR dstColorBlendFactor{graphics::BLEND_FACTOR::ZERO};

    graphics::BLEND_OPERATION colorBlendOperation{graphics::BLEND_OPERATION::ADD};

    graphics::BLEND_FACTOR srcAlphaBlendFactor{graphics::BLEND_FACTOR::ONE};
    graphics::BLEND_FACTOR dstAlphaBlendFactor{graphics::BLEND_FACTOR::ZERO};

    graphics::BLEND_OPERATION alphaBlendOperation{graphics::BLEND_OPERATION::ADD};

    graphics::COLOR_COMPONENT colorWriteMask{graphics::COLOR_COMPONENT::RGBA};
};

struct ColorBlendState final {
    bool logicOperationEnable{false};

    graphics::BLEND_STATE_OPERATION logicOperation{graphics::BLEND_STATE_OPERATION::COPY};

    std::array<float, 4> blendConstants{0, 0, 0, 0};

    std::vector<ColorBlendAttachmentState> attachments;
};



namespace graphics
{
    struct color_attachment final {
        graphics::FORMAT format;
    };

    struct depth_stencil_attachment final {
        graphics::FORMAT format;
    };

    using attachment = std::variant<graphics::color_attachment, graphics::depth_stencil_attachment>;

    struct attachment_description final {
        graphics::FORMAT format;
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
