#pragma once

#include <vector>
#include <memory>

#include "graphics.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"


namespace graphics
{
    struct descriptor_set_binding final {
        // TODO:: immutable samplers support.

        std::uint32_t binding_index;
        std::uint32_t descriptor_count;

        graphics::DESCRIPTOR_TYPE descriptor_type;
        graphics::SHADER_STAGE shader_stages;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, descriptor_set_binding>
        auto constexpr operator== (T &&rhs) const
        {
            return binding_index == rhs.binding_index &&
                descriptor_count == rhs.descriptor_count &&
                descriptor_type == rhs.descriptor_type &&
                shader_stages == rhs.shader_stages;
        }
    };

    class descriptor_set_layout final {
    public:

        descriptor_set_layout(VkDescriptorSetLayout handle) noexcept : handle_{handle} { }

        VkDescriptorSetLayout handle() const noexcept { return handle_; }

        std::vector<graphics::descriptor_set_binding> const &descriptor_set_bindings() const noexcept { return descriptor_set_bindings_; }

    private:
        // TODO:: create flags support.

        VkDescriptorSetLayout handle_{VK_NULL_HANDLE};

        std::vector<graphics::descriptor_set_binding> descriptor_set_bindings_;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, descriptor_set_layout>
        auto constexpr operator== (T &&rhs) const
        {
            return descriptor_set_bindings_ == rhs.descriptor_set_bindings_;
        }
    };
}

namespace graphics
{
    class descriptor_registry final {
    public:

        static auto constexpr kDESCRIPTOR_SET_MAXIMUM_NUMBER{128u};

        descriptor_registry(vulkan::device &device);
        ~descriptor_registry();

        [[nodiscard]] std::shared_ptr<descriptor_set_layout>
        create_descriptor_set_layout(std::vector<graphics::descriptor_set_binding> const &descriptor_set_bindings);

    private:

        vulkan::device &device_;

        VkDescriptorPool descriptor_pool_;
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::descriptor_set_binding> {
        std::size_t operator() (graphics::descriptor_set_binding const &binding) const;
    };

    template<>
    struct hash<graphics::descriptor_set_layout> {
        std::size_t operator() (graphics::descriptor_set_layout const &binding) const;
    };
}
