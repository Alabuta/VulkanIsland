#pragma once

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
#endif


namespace vulkan
{
    [[nodiscard]] VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object, std::size_t location,
                   std::int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);

    [[nodiscard]] VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_types,
                   VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
                   void *user_data);

    void create_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT &callback);
}
