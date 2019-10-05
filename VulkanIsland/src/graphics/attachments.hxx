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

        graphics::ATTACHMENT_LOAD_TREATMENT load_op;
        graphics::ATTACHMENT_STORE_TREATMENT store_op;

        // At the begining of a render pass.
        graphics::IMAGE_LAYOUT initial_layout;
        // At the end of a render pass.
        graphics::IMAGE_LAYOUT final_layout;

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

        // During a subpass.
        graphics::IMAGE_LAYOUT subpass_layout;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, attachment_reference>
        auto constexpr operator== (T &&rhs) const
        {
            return pass_index == rhs.pass_index && attachment_index == rhs.attachment_index && subpass_layout == rhs.subpass_layout;
        }
    };

    using attachment = std::variant<graphics::attachment_description, graphics::attachment_reference>;
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
