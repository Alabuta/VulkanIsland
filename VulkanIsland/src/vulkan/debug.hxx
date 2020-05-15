#pragma once

#include <volk/volk.h>


namespace vulkan
{
    VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                         VkDebugUtilsMessageTypeFlagsEXT message_type,
                         const VkDebugUtilsMessengerCallbackDataEXT *data, void *user_data);

    VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object, std::size_t location,
                   std::int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);

    void create_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT &callback);
}
