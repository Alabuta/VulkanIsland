#pragma once

#include <vector>
#include <array>
#include <string>
#include <string_view>


template<bool USE_DEBUG_LAYERS>
class VulkanInstance final {
public:

    template<class E, class L>
    VulkanInstance(E &&extensions, L &&layers)
    {
        static_assert(is_container_v<std::decay_t<E>>, "'extensions' variables must be a container");
        static_assert(std::is_same_v<typename std::decay_t<E>::value_type, char const *>, "'extensions' must contain null-terminated strings");

        if constexpr (USE_DEBUG_LAYERS)
        {
            static_assert(is_container_v<std::decay_t<L>>, "'layers' variables must be a container");
            static_assert(std::is_same_v<typename std::decay_t<L>::value_type, char const *>, "'layers' must contain null-terminated strings");
        }

        if constexpr (USE_DEBUG_LAYERS)
            if (auto supported = CheckRequiredLayers(layers); !supported)
                throw std::runtime_error("not all required layers are supported"s);

        if (auto supported = CheckRequiredExtensions(extensions); !supported)
            throw std::runtime_error("not all required extensions are supported"s);

        VkInstanceCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr, 0,
            &app_info,
            USE_DEBUG_LAYERS ? static_cast<std::uint32_t>(std::size(layers)) : 0, std::data(layers),
            static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions)
        };

        if (auto result = vkCreateInstance(&createInfo, nullptr, &instance_); result != VK_SUCCESS)
            throw std::runtime_error("failed to create instance"s);

        if constexpr (USE_DEBUG_LAYERS)
            CreateDebugReportCallback(instance_, debugReportCallback_);
    }

    ~VulkanInstance()
    {
        if constexpr (USE_DEBUG_LAYERS)
            vkDestroyDebugReportCallbackEXT(instance_, debugReportCallback_, nullptr);

        vkDestroyInstance(instance_, nullptr);
    }

    VkInstance instance_{VK_NULL_HANDLE};

private:
    VkDebugReportCallbackEXT debugReportCallback_{VK_NULL_HANDLE};

    VulkanInstance() = delete;

    VulkanInstance(VulkanInstance const &) = delete;
    VulkanInstance(VulkanInstance &&) = default;
};