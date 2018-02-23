
#include "main.h"
#include "instance.h"
#include "device.h"

VulkanInstance::~VulkanInstance()
{
    if (debugReportCallback_ != VK_NULL_HANDLE)
        vkDestroyDebugReportCallbackEXT(instance_, debugReportCallback_, nullptr);

    debugReportCallback_ = VK_NULL_HANDLE;

    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
}

void VulkanInstance::CreateInstance(std::vector<char const *> &&extensions, std::vector<char const *> &&layers)
{
    VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &app_info,
        0, nullptr,
        0, nullptr
    };

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported"s);

    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
    createInfo.ppEnabledExtensionNames = std::data(extensions);

    if (auto supported = CheckRequiredLayers(layers); !supported)
        throw std::runtime_error("not all required layers are supported"s);

    createInfo.enabledLayerCount = static_cast<std::uint32_t>(std::size(layers));
    createInfo.ppEnabledLayerNames = std::data(layers);

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance_); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

    if (!layers.empty())
        CreateDebugReportCallback(instance_, debugReportCallback_);
}