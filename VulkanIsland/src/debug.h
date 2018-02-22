#pragma once

#include "debug.h"


[[nodiscard]] inline VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object, std::size_t location, std::int32_t messageCode,
              const char *pLayerPrefix, const char *pMessage, void *pUserData);

void CreateDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT &callback);