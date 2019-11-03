#include <exception>
#include <algorithm>
#include <vector>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "vulkan_config.hxx"
#include "instance.hxx"


namespace
{
    [[nodiscard]] bool check_required_extensions(std::vector<char const *> _required_extensions)
    {
        std::vector<VkExtensionProperties> required_extensions;

        std::transform(std::cbegin(_required_extensions), std::cend(_required_extensions), 
                       std::back_inserter(required_extensions), [] (auto &&name)
        {
            VkExtensionProperties prop{};
            std::uninitialized_copy_n(name, std::strlen(name), prop.extensionName);

            return prop;
        });

        auto constexpr extensions_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName),
                                                std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
        };

        std::sort(std::begin(required_extensions), std::end(required_extensions), extensions_compare);

        std::uint32_t extensions_count = 0;

        if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve extensions count: {0:#x}\n"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve extensions: {0:#x}\n"s, result));

        std::sort(std::begin(supported_extensions), std::end(supported_extensions), extensions_compare);

        return std::includes(std::cbegin(supported_extensions), std::cend(supported_extensions),
                             std::cbegin(required_extensions), std::cend(required_extensions), extensions_compare);
    }

    [[nodiscard]] bool check_required_layers(std::vector<char const *> _requiredLayers)
    {
        std::vector<VkLayerProperties> required_layers;

        std::transform(std::cbegin(_requiredLayers), std::cend(_requiredLayers), std::back_inserter(required_layers), [] (auto &&name)
        {
            VkLayerProperties prop{};
            std::uninitialized_copy_n(name, std::strlen(name), prop.layerName);

            return prop;
        });

        auto constexpr layers_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::lexicographical_compare(std::cbegin(lhs.layerName), std::cend(lhs.layerName), std::cbegin(rhs.layerName), std::cend(rhs.layerName));
        };

        std::sort(std::begin(required_layers), std::end(required_layers), layers_compare);

        std::uint32_t layers_count = 0;

        if (auto result = vkEnumerateInstanceLayerProperties(&layers_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve layers count: {0:#x}\n"s, result));

        std::vector<VkLayerProperties> supportedLayers(layers_count);

        if (auto result = vkEnumerateInstanceLayerProperties(&layers_count, std::data(supportedLayers)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve layers: {0:#x}\n"s, result));

        std::sort(std::begin(supportedLayers), std::end(supportedLayers), layers_compare);

        return std::includes(supportedLayers.begin(), supportedLayers.end(), required_layers.begin(), required_layers.end(), layers_compare);
    }
}

namespace vulkan
{
    instance::instance()
    {
        auto constexpr use_extensions = !vulkan_config::extensions.empty();
        auto constexpr use_layers = !vulkan_config::layers.empty();

        std::vector<char const *> extensions;
        std::vector<char const *> layers;

        if constexpr (use_extensions) {
            auto _extensions = vulkan_config::extensions;

            if constexpr (use_layers) {
                auto present = std::any_of(std::cbegin(_extensions), std::cend(_extensions), [] (auto &&name)
                {
                    return std::strcmp(name, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0;
                });

                if (!present)
                    throw std::runtime_error("enabled validation layers require enabled 'VK_EXT_debug_report' extension"s);
            }

            std::copy(std::cbegin(_extensions), std::cend(_extensions), std::back_inserter(extensions));
        }

        if constexpr (use_layers) {
            auto _layers = vulkan_config::layers;

            std::copy(std::cbegin(_layers), std::cend(_layers), std::back_inserter(layers));
        }

        std::uint32_t api_version = 0;

        if (auto result = vkEnumerateInstanceVersion(&api_version); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve Vulkan API version"s);

        auto const application_info = vulkan_config::application_info;

        auto api_major = VK_VERSION_MAJOR(api_version);
        auto api_minor = VK_VERSION_MINOR(api_version);

        auto required_major = VK_VERSION_MAJOR(application_info.apiVersion);
        auto required_minor = VK_VERSION_MINOR(application_info.apiVersion);

        if (api_major != required_major || api_minor != required_minor)
            throw std::runtime_error("unsupported Vulkan API version"s);

        VkInstanceCreateInfo instance_info{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr, 0,
            &application_info,
            0, nullptr,
            0, nullptr
        };

        if (auto supported = check_required_extensions(extensions); !supported)
            throw std::runtime_error("not all required extensions are supported"s);

        instance_info.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
        instance_info.ppEnabledExtensionNames = std::data(extensions);

        if (auto supported = check_required_layers(layers); !supported)
            throw std::runtime_error("not all required layers are supported"s);

        instance_info.enabledLayerCount = static_cast<std::uint32_t>(std::size(layers));
        instance_info.ppEnabledLayerNames = std::data(layers);

        if (auto result = vkCreateInstance(&instance_info, nullptr, &instance_); result != VK_SUCCESS)
            throw std::runtime_error("failed to create instance"s);

        if constexpr (use_layers)
            vulkan::create_debug_report_callback(instance_, debug_report_callback_);
    }

    instance::~instance()
    {
        if (debug_report_callback_ != VK_NULL_HANDLE)
            vkDestroyDebugReportCallbackEXT(instance_, debug_report_callback_, nullptr);

        debug_report_callback_ = VK_NULL_HANDLE;

        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}