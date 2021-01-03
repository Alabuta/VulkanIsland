#include <iostream>
#include <cstddef>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/exceptions.hxx"
#include "debug.hxx"


namespace vulkan
{
    VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_utils_callback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                         [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
                         VkDebugUtilsMessengerCallbackDataEXT const *data, [[maybe_unused]] void *user_data)
    {
        using namespace std::string_literals;

        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            fmt::print(stderr, "{} - {} : {}\n\n"s, data->messageIdNumber, data->pMessageIdName, data->pMessage);

        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            fmt::print(stderr, "{} - {} : {}\n\n"s, data->messageIdNumber, data->pMessageIdName, data->pMessage);

        return VK_FALSE;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_report_callback([[maybe_unused]] VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT object_type,
                   [[maybe_unused]] std::uint64_t object, [[maybe_unused]] std::size_t location, [[maybe_unused]] std::int32_t message_code,
                   [[maybe_unused]] const char *layer_prefix, const char *message, [[maybe_unused]] void *user_data)
    {
        fmt::print(stderr, "{}\n\n"s, message);

        return VK_FALSE;
    }

    void create_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT &callback)
    {
        using namespace std::string_literals;

        VkDebugReportCallbackCreateInfoEXT constexpr create_info{
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            nullptr,
            VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
            debug_report_callback,
            nullptr
        };

        if (auto result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &callback); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to set up debug callback: {0:#x}"s, result));
    }
}
