#include "image.h"

[[nodiscard]] std::optional<VkImage>
CreateImageHandle(VulkanDevice &vulkanDevice, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                              VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage)
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        format,
        { width, height, 1 },
        mipLevels,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        tiling,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkImage handle;

    if (auto result = vkCreateImage(vulkanDevice.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image: "s + std::to_string(result));

#if USE_DEBUG_MARKERS
    VkDebugMarkerObjectNameInfoEXT const info{
        VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
        (std::uint64_t)handle,//static_cast<std::uint64_t>(handle),
        "image"
    };

    auto vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(vulkanDevice.handle(), "vkDebugMarkerSetObjectNameEXT");

    if (auto result = vkDebugMarkerSetObjectNameEXT(vulkanDevice.handle(), &info); result != VK_SUCCESS)
        throw std::runtime_error("failed to set the image debug marker: "s + std::to_string(result));
#endif

    return handle;
}

