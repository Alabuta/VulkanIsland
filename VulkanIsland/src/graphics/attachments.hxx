#pragma once

#include <array>
#include <vector>
#include <variant>

#include "utility/mpl.hxx"
#include "graphics.hxx"


namespace resource
{
    class image;
    class image_view;
}

namespace graphics
{
    struct attachment_description final {
        graphics::FORMAT format;

        std::uint32_t samples_count;

        graphics::ATTACHMENT_LOAD_TREATMENT load_op;      // At the begining of the subpass where it first used.
        graphics::ATTACHMENT_STORE_TREATMENT store_op;    // At the end of the subpass where it last used.

        graphics::IMAGE_LAYOUT initial_layout;      // At the begining of the render pass.
        graphics::IMAGE_LAYOUT final_layout;        // At the end of the render pass.

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, attachment_description>
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

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, attachment_reference>
        auto constexpr operator== (T &&rhs) const
        {
            return pass_index == rhs.pass_index && attachment_index == rhs.attachment_index && subpass_layout == rhs.subpass_layout;
        }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, attachment_reference>
        auto constexpr operator< (T &&rhs) const
        {
            return attachment_index < rhs.attachment_index;
        }
    };

    struct framebuffer_attachment {
        graphics::FORMAT format;
        graphics::IMAGE_TILING tiling;

        std::uint32_t mip_levels;
        std::uint32_t samples_count;

        std::shared_ptr<resource::image> image;
        std::shared_ptr<resource::image_view> image_view;
    };

    struct color_attachment final : public framebuffer_attachment { };
    struct depth_stencil_attachment final : public framebuffer_attachment { };

    using attachment = std::variant<graphics::color_attachment, graphics::depth_stencil_attachment>;
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
