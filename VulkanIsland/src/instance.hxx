#pragma once

#include <vector>
#include <array>
#include <cstring>
#include <string>
#include <string_view>
#include <algorithm>

#include "helpers.hxx"

#define GLFW_EXPOSE_NATIVE_WIN32

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "debug.hxx"


namespace config {
auto constexpr extensions = make_array(
    VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _MSC_VER
#if USE_WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
    "VK_KHR_win32_surface",
#endif
#else
    "VK_KHR_xcb_surface",
#endif
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
);

auto constexpr layers = make_array(
#if TEMPORARILY_DISABLED
    "VK_LAYER_NV_nsight",
    "VK_LAYER_LUNARG_api_dump",
#endif

    "VK_LAYER_LUNARG_assistant_layer",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_GOOGLE_unique_objects"
);
}

class VulkanInstance final {
public:

    template<class E, class L>
    VulkanInstance(E &&extensions, L &&layers);
    ~VulkanInstance();

    VkInstance handle() const noexcept { return instance_; }
         
private:

    VkInstance instance_{nullptr};
    VkDebugReportCallbackEXT debugReportCallback_{nullptr};

    VulkanInstance() = delete;
    VulkanInstance(VulkanInstance const &) = delete;
    VulkanInstance(VulkanInstance &&) = delete;

    void CreateInstance(std::vector<char const *> &&extensions, std::vector<char const *> &&layers);
};

template<class E, class L>
inline VulkanInstance::VulkanInstance(E &&extensions, L &&layers)
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    auto constexpr use_extensions = !std::is_same_v<std::false_type, E>;
    auto constexpr use_layers = !std::is_same_v<std::false_type, L>;

    std::vector<char const *> extensions_;
    std::vector<char const *> layers_;

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

        if constexpr (std::is_rvalue_reference_v<T>)
            std::move(extensions.begin(), extensions.end(), std::back_inserter(extensions_));

        else std::copy(extensions.begin(), extensions.end(), std::back_inserter(extensions_));

    }

    if constexpr (use_layers)
    {
        using T = std::decay_t<L>;
        static_assert(is_container_v<T>, "'layers' must be a container");
        static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "'layers' must contain null-terminated strings");

        if constexpr (std::is_rvalue_reference_v<T>)
            std::move(layers.begin(), layers.end(), std::back_inserter(layers_));

        else std::copy(layers.begin(), layers.end(), std::back_inserter(layers_));
    }

    CreateInstance(std::move(extensions_), std::move(layers_));
}