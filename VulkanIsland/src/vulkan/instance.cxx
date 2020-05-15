#include <exception>
#include <algorithm>
#include <iostream>
#include <vector>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/exceptions.hxx"
#include "vulkan_config.hxx"
#include "instance.hxx"


namespace
{
    [[nodiscard]] bool check_required_extensions(std::vector<char const *> required_extensions_)
    {
        std::vector<VkExtensionProperties> required_extensions;

        std::transform(std::cbegin(required_extensions_), std::cend(required_extensions_), 
                       std::back_inserter(required_extensions), [] (auto &&name)
        {
            VkExtensionProperties prop{};
            std::uninitialized_copy_n(name, std::strlen(name), prop.extensionName);

            return prop;
        });

        auto extensions_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::string_view{lhs.extensionName} < std::string_view{rhs.extensionName};
        };

        std::sort(std::begin(required_extensions), std::end(required_extensions), extensions_compare);

        std::uint32_t extensions_count = 0;

        if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr); result != VK_SUCCESS)
            throw vulkan::instance_exception(fmt::format("failed to retrieve extensions count: {0:#x}"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw vulkan::instance_exception(fmt::format("failed to retrieve extensions: {0:#x}"s, result));

        std::sort(std::begin(supported_extensions), std::end(supported_extensions), extensions_compare);

        std::vector<VkExtensionProperties> unsupported_extensions;

        std::set_difference(std::begin(required_extensions), std::end(required_extensions), std::begin(supported_extensions),
                            std::end(supported_extensions), std::back_inserter(unsupported_extensions), extensions_compare);

        if (unsupported_extensions.empty())
            return true;

        std::cerr << "unsupported instance extensions: "s << std::endl;

        for (auto &&extension : unsupported_extensions)
            std::cerr << fmt::format("{}\n"s, extension.extensionName);

        return false;
    }

    [[nodiscard]] bool check_required_layers(std::vector<char const *> required_layers_)
    {
        std::vector<VkLayerProperties> required_layers;

        std::transform(std::cbegin(required_layers_), std::cend(required_layers_), std::back_inserter(required_layers), [] (auto &&name)
        {
            VkLayerProperties prop{};
            std::uninitialized_copy_n(name, std::strlen(name), prop.layerName);

            return prop;
        });

        auto layers_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::lexicographical_compare(std::cbegin(lhs.layerName), std::cend(lhs.layerName),
                                                std::cbegin(rhs.layerName), std::cend(rhs.layerName));
        };

        std::sort(std::begin(required_layers), std::end(required_layers), layers_compare);

        std::uint32_t layers_count = 0;

        if (auto result = vkEnumerateInstanceLayerProperties(&layers_count, nullptr); result != VK_SUCCESS)
            throw vulkan::instance_exception(fmt::format("failed to retrieve layers count: {0:#x}"s, result));

        std::vector<VkLayerProperties> supported_layers(layers_count);

        if (auto result = vkEnumerateInstanceLayerProperties(&layers_count, std::data(supported_layers)); result != VK_SUCCESS)
            throw vulkan::instance_exception(fmt::format("failed to retrieve layers: {0:#x}"s, result));

        std::sort(std::begin(supported_layers), std::end(supported_layers), layers_compare);

        std::vector<VkLayerProperties> unsupported_layers;

        std::set_difference(std::begin(required_layers), std::end(required_layers), std::begin(supported_layers),
                            std::end(supported_layers), std::back_inserter(unsupported_layers), layers_compare);

        if (unsupported_layers.empty())
            return true;

        std::cerr << "unsupported instance layers: "s << std::endl;

        for (auto &&layer : unsupported_layers)
            std::cerr << fmt::format("{}\n"s, layer.layerName);

        return false;
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
            auto extensions_ = vulkan_config::extensions;

            if constexpr (use_layers) {
                auto present = std::any_of(std::cbegin(extensions_), std::cend(extensions_), [] (auto extension)
                {
                #if PREFER_DEBUG_UTILS
                    return std::strcmp(extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
                #else
                    return std::strcmp(extension, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0;
                #endif
                });

                if (!present)
                #if PREFER_DEBUG_UTILS
                    throw vulkan::logic_error("validation layers require enabled 'VK_EXT_debug_utils' extension"s);
                #else
                    throw vulkan::logic_error("validation layers require enabled 'VK_EXT_debug_report' extension"s);
                #endif
            }

            std::copy(std::cbegin(extensions_), std::cend(extensions_), std::back_inserter(extensions));
        }

        if constexpr (use_layers) {
            auto layers_ = vulkan_config::layers;

            std::copy(std::cbegin(layers_), std::cend(layers_), std::back_inserter(layers));
        }

        std::uint32_t api_version = 0;

        if (auto result = vkEnumerateInstanceVersion(&api_version); result != VK_SUCCESS)
            throw vulkan::instance_exception("failed to retrieve Vulkan API version"s);

        auto const application_info = vulkan_config::application_info;

        auto supported_api_major = VK_VERSION_MAJOR(api_version);
        auto supported_api_minor = VK_VERSION_MINOR(api_version);

        auto required_api_major = VK_VERSION_MAJOR(application_info.apiVersion);
        auto required_api_minor = VK_VERSION_MINOR(application_info.apiVersion);

        if (supported_api_major != required_api_major || supported_api_minor != required_api_minor)
            throw vulkan::instance_exception("unsupported Vulkan API version"s);

        VkInstanceCreateInfo create_info{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr, 0,
            &application_info,
            0, nullptr,
            0, nullptr
        };

    #if PREFER_DEBUG_UTILS
        VkDebugUtilsMessengerCreateInfoEXT debug_msg_create_info{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            nullptr, 0,
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            debug_utils_callback,
            nullptr
        };
    #endif

        if constexpr (use_layers) {
            auto const enabled_validation_features = std::array{
                //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
                VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
                //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
            };

            VkValidationFeaturesEXT const validation_features{
                VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                nullptr,
                static_cast<std::uint32_t>(std::size(enabled_validation_features)),
                std::data(enabled_validation_features),
                0, nullptr
            };

        #if PREFER_DEBUG_UTILS
            debug_msg_create_info.pNext = &validation_features;
            create_info.pNext = &debug_msg_create_info;
        #else
            create_info.pNext = &validation_features;
        #endif
        }

        if (auto supported = check_required_extensions(extensions); !supported)
            throw vulkan::instance_exception("not all required extensions are supported"s);

        create_info.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
        create_info.ppEnabledExtensionNames = std::data(extensions);

        if (auto supported = check_required_layers(layers); !supported)
            throw vulkan::instance_exception("not all required layers are supported"s);

        create_info.enabledLayerCount = static_cast<std::uint32_t>(std::size(layers));
        create_info.ppEnabledLayerNames = std::data(layers);

        if (auto result = vkCreateInstance(&create_info, nullptr, &handle_); result != VK_SUCCESS)
            throw vulkan::instance_exception("failed to create instance"s);

        if constexpr (use_layers) {
        #if PREFER_DEBUG_UTILS
            if (auto result = vkCreateDebugUtilsMessengerEXT(handle_, &debug_msg_create_info, nullptr, &debug_messenger_); result != VK_SUCCESS)
                throw std::runtime_error(fmt::format("failed to set up debug messenger: {0:#x}\n"s, result));
        #else
            vulkan::create_debug_report_callback(handle_, debug_report_callback_);
        #endif
        }
    }

    instance::~instance()
    {
        if (handle_ == VK_NULL_HANDLE)
            return;

        for (auto [window_handle, platform_surface] : platform_surfaces_)
            vkDestroySurfaceKHR(handle_, platform_surface.handle(), nullptr);

        if (debug_messenger_ != VK_NULL_HANDLE)
            vkDestroyDebugUtilsMessengerEXT(handle_, debug_messenger_, nullptr);

        debug_messenger_ = VK_NULL_HANDLE;

        if (debug_report_callback_ != VK_NULL_HANDLE)
            vkDestroyDebugReportCallbackEXT(handle_, debug_report_callback_, nullptr);

        debug_report_callback_ = VK_NULL_HANDLE;

        vkDestroyInstance(handle_, nullptr);

        handle_ = VK_NULL_HANDLE;
    }

    renderer::platform_surface instance::get_platform_surface(platform::window &window)
    {
        if (!platform_surfaces_.contains(window.handle())) {
            renderer::platform_surface platform_surface;

            if (auto result = glfwCreateWindowSurface(handle_, window.handle(), nullptr, &platform_surface.handle_); result != VK_SUCCESS)
                throw std::runtime_error(fmt::format("failed to create window surface: {0:#x}\n"s, result));

            platform_surfaces_.emplace(window.handle(), platform_surface);
        }

        return platform_surfaces_.at(window.handle());
    }
}