#pragma once

#include <array>
#include <vector>
#include <variant>

#include "utility/mpl.hxx"
#include "graphics.hxx"


namespace graphics
{
    struct attachment_description final {
        graphics::FORMAT format;
        std::uint32_t samples_count;

        graphics::ATTACHMENT_LOAD_TREATMENT load_op;      // At the begining of the subpass where it first used.
        graphics::ATTACHMENT_STORE_TREATMENT store_op;    // At the end of the subpass where it last used.

        graphics::IMAGE_LAYOUT initial_layout;      // At the begining of the render pass.
        graphics::IMAGE_LAYOUT final_layout;        // At the end of the render pass.

        template<class T> requires std::same_as<std::remove_cvref_t<T>, attachment_description>
        auto constexpr operator== (T &&rhs) const
        {
            return format == rhs.format && samples_count == rhs.samples_count &&
                load_op == rhs.load_op && store_op == rhs.store_op &&
                initial_layout == rhs.initial_layout && final_layout == rhs.final_layout;
        }
    };

    struct attachment_reference final {
        std::uint32_t pass_index;
        std::uint32_t attachment_index;

        graphics::IMAGE_LAYOUT subpass_layout;      // During a subpass.

        template<class T> requires std::same_as<std::remove_cvref_t<T>, attachment_reference>
        auto constexpr operator== (T &&rhs) const
        {
            return pass_index == rhs.pass_index && attachment_index == rhs.attachment_index && subpass_layout == rhs.subpass_layout;
        }

        template<class T> requires std::same_as<std::remove_cvref_t<T>, attachment_reference>
        auto constexpr operator< (T &&rhs) const
        {
            return attachment_index < rhs.attachment_index;
        }
    };

    struct color_attachment final {
        static auto constexpr usage{graphics::IMAGE_USAGE::COLOR_ATTACHMENT};
        static auto constexpr layout{graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT};
        static auto constexpr aspect{graphics::IMAGE_ASPECT::COLOR_BIT};
    };

    struct depth_attachment final {
        static auto constexpr usage{graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT};
        static auto constexpr layout{graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT};
        static auto constexpr aspect{graphics::IMAGE_ASPECT::DEPTH_BIT};
    };

    struct depth_stencil_attachment final {
        static auto constexpr usage{graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT};
        static auto constexpr layout{graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT};
        static auto constexpr aspect{graphics::IMAGE_ASPECT::DEPTH_BIT | graphics::IMAGE_ASPECT::STENCIL_BIT};
    };

    using attachment = std::variant<graphics::color_attachment, graphics::depth_attachment, graphics::depth_stencil_attachment>;
}

namespace graphics
{
    template<>
    struct hash<graphics::attachment_description> {
        std::size_t operator() (graphics::attachment_description const &attachment_description) const;
    };

    template<>
    struct hash<graphics::attachment_reference> {
        std::size_t operator() (graphics::attachment_reference const &attachment_reference) const;
    };
}
