#pragma once

#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
#include <string_view>

#include "utility/mpl.hxx"

#define GLFW_EXPOSE_NATIVE_WIN32

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "vulkan_config.hxx"
#include "debug.hxx"


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

    auto constexpr use_extensions = !std::is_same_v<std::false_type, E>;
    auto constexpr use_layers = !std::is_same_v<std::false_type, L>;

    std::vector<char const *> extensions_;
    std::vector<char const *> layers_;

    if constexpr (use_extensions) {
        using T = std::remove_cvref_t<E>;

        static_assert(mpl::container<T>, "'extensions' must be a container");
        static_assert(std::same_as<typename std::remove_cvref_t<T>::value_type, char const *>, "'extensions' must contain null-terminated strings");

        if constexpr (use_layers) {
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

    if constexpr (use_layers) {
        using T = std::remove_cvref_t<L>;

        static_assert(mpl::container<T>, "'layers' must be a container");
        static_assert(std::same_as<typename std::remove_cvref_t<T>::value_type, char const *>, "'layers' must contain null-terminated strings");

        if constexpr (std::is_rvalue_reference_v<T>)
            std::move(layers.begin(), layers.end(), std::back_inserter(layers_));

        else std::copy(layers.begin(), layers.end(), std::back_inserter(layers_));
    }

    CreateInstance(std::move(extensions_), std::move(layers_));
}