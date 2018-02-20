#pragma once

#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <mutex>
#include <optional>

#include <gsl\gsl>

#include "helpers.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

/*template<bool USE_DEBUG_LAYERS>
class VulkanInstance<USE_DEBUG_LAYERS>::VulkanDevice;*/


class VulkanInstance final {
public:

    VkInstance instance_{VK_NULL_HANDLE};

    template<class E, class L>
    VulkanInstance(E &&extensions, L &&layers);
    ~VulkanInstance();

    VkInstance handle() noexcept;

    /*class VulkanDevice;
    VulkanInstance<USE_DEBUG_LAYERS>::VulkanDevice &device();*/

private:
    VkDebugReportCallbackEXT debugReportCallback_{VK_NULL_HANDLE};

    /*std::once_flag device_setup_;
    VulkanDevice device_;*/

    VulkanInstance() = delete;
    VulkanInstance(VulkanInstance const &) = delete;
    VulkanInstance(VulkanInstance &&) = delete;
};

template<class E, class L>
inline VulkanInstance::VulkanInstance(E &&extensions, L &&layers)
{
    auto constexpr use_extensions = !std::is_same_v<std::false_type, E>;
    auto constexpr use_layers = !std::is_same_v<std::false_type, L>;

    VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &app_info,
        0, nullptr,
        0, nullptr
    };

    if constexpr (use_extensions)
    {
        using T = std::decay_t<E>;
        static_assert(is_container_v<T>, "'extensions' must be a container");
        static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "'extensions' must contain null-terminated strings");

        if constexpr (use_layers)
        {
            auto present = std::any_of(std::cbegin(extensions), std::cend(extensions), [] (auto &&name)
            {
                return std::strcmp(name, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0;
            });

            if (!present)
                throw std::runtime_error("enabled validation layers require enabled 'VK_EXT_debug_report' extension"s);
        }

        if (auto supported = CheckRequiredExtensions(extensions); !supported)
            throw std::runtime_error("not all required extensions are supported"s);

        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
        createInfo.ppEnabledExtensionNames = std::data(extensions);
    }

    if constexpr (use_layers)
    {
        using T = std::decay_t<L>;
        static_assert(is_container_v<T>, "'layers' must be a container");
        static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "'layers' must contain null-terminated strings");

        if (auto supported = CheckRequiredLayers(layers); !supported)
            throw std::runtime_error("not all required layers are supported"s);

        createInfo.enabledLayerCount = static_cast<std::uint32_t>(std::size(layers));
        createInfo.ppEnabledLayerNames = std::data(layers);
    }

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance_); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

    if constexpr (use_layers)
        CreateDebugReportCallback(instance_, debugReportCallback_);
}

inline VkInstance VulkanInstance::handle() noexcept
{
    return instance_;
}


/*template<bool USE_DEBUG_LAYERS>
typename VulkanInstance<USE_DEBUG_LAYERS>::VulkanDevice &VulkanInstance<USE_DEBUG_LAYERS>::device()
{
    std::call_once(device_setup_, [&device_] ()
    {
        device_ = std::move(VulkanDevice());
    });

    return device_;
}*/