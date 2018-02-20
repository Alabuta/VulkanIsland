#include "instance.h"

#include "device.h"

VulkanInstance::~VulkanInstance()
{
    if (debugReportCallback_ != VK_NULL_HANDLE)
        vkDestroyDebugReportCallbackEXT(instance_, debugReportCallback_, nullptr);

    vkDestroyInstance(instance_, nullptr);
}