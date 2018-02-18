#pragma once

#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <mutex>
#include <optional>

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
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};

    template<class E, class... Os>
    VulkanInstance(E &&extensions, Os &&... optional);

    ~VulkanInstance();


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

template<class E, class... Os>
VulkanInstance::VulkanInstance(E &&extensions, Os &&... optional)
{
    static_assert(is_container_v<std::decay_t<E>>, "'extensions' must be a container");
    static_assert(std::is_same_v<typename std::decay_t<E>::value_type, char const *>, "'extensions' must contain null-terminated strings");

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported"s);

    std::vector<char const*> layers;

    if constexpr (sizeof...(Os))
    {
        static_assert(sizeof...(Os) == 1, "just one optional argument is supported");

        auto tuple = std::make_tuple("", std::forward<Os>(optional)...);

        if constexpr (std::tuple_size_v<decltype(tuple)> == 2)
        {
            using L = std::decay_t<std::tuple_element_t<0, std::tuple<Os...>>>;

            static_assert(is_container_v<L> && is_iterable_v<L>, "optional argument must be an iterable container");
            static_assert(std::is_same_v<typename L::value_type, char const *>, "optional argument must contain null-terminated strings");

            auto temp = std::move(std::get<1>(tuple));
            std::copy_n(std::begin(temp), std::size(temp), std::back_inserter(layers));

            if (auto supported = CheckRequiredLayers(layers); !supported)
                throw std::runtime_error("not all required layers are supported"s);
        }
    }

    VkInstanceCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &app_info,
        static_cast<std::uint32_t>(std::size(layers)), std::data(layers),
        static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions)
    };

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance_); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

    if constexpr (sizeof...(Os))
        CreateDebugReportCallback(instance_, debugReportCallback_);
}


inline VulkanInstance::~VulkanInstance()
{
    if (debugReportCallback_ != VK_NULL_HANDLE)
        vkDestroyDebugReportCallbackEXT(instance_, debugReportCallback_, nullptr);

    vkDestroyInstance(instance_, nullptr);
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