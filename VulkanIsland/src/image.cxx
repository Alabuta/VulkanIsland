#include "image.h"


[[nodiscard]] std::optional<VkFormat> FindDepthImageFormat(VkPhysicalDevice physicalDevice) noexcept
{
    return FindSupportedImageFormat(
        physicalDevice,
        make_array(VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

[[nodiscard]] std::optional<VkImage>
CreateImageHandle(VulkanDevice const &vulkanDevice, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                              VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage) noexcept
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

    std::optional<VkImage> image;

    VkImage handle;

    if (auto result = vkCreateImage(vulkanDevice.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create image: "s << result << '\n';

    else image.emplace(handle);

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

    return image;
}


[[nodiscard]] std::optional<VulkanImage>
CreateImage(VulkanDevice &device, VkFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
            VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::optional<VulkanImage> image;

    auto handle = CreateImageHandle(device, width, height, mipLevels, format, tiling, usageFlags);

    if (handle) {
        auto memory = device.memoryManager().AllocateMemory(*handle, propertyFlags, tiling == VK_IMAGE_TILING_LINEAR);

        if (memory) {
            if (auto result = vkBindImageMemory(device.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind image buffer memory: "s << result << '\n';

            else image.emplace(*handle, memory, format, mipLevels, width, height);
        }
    }

    return image;
}

[[nodiscard]] std::optional<VulkanImageView>
CreateImageView(VulkanDevice const &device, VulkanImage const &image, VkImageAspectFlags aspectFlags) noexcept
{
    std::optional<VulkanImageView> view;

    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image.handle,
        VK_IMAGE_VIEW_TYPE_2D,
        image.format,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, image.mipLevels, 0, 1 }
    };

    VkImageView handle;

    if (auto result = vkCreateImageView(device.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create image view: "s << result << '\n';

    else view.emplace(handle, image.format);

    return view;
}

[[nodiscard]] std::optional<VulkanSampler> CreateTextureSampler(VkDevice device, std::uint32_t mipLevels) noexcept
{
    std::optional<VulkanSampler> sampler;

    VkSamplerCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr, 0,
        VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.f,
        VK_TRUE, 16.f,
        VK_FALSE, VK_COMPARE_OP_ALWAYS,
        0.f, static_cast<float>(mipLevels),
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_FALSE
    };

    VkSampler handle;

    if (auto result = vkCreateSampler(device, &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create sampler: "s << result << '\n';

    else sampler.emplace(handle);

    return sampler;
}


[[nodiscard]] std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, VkFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels, VkImageTiling tiling,
              VkImageAspectFlags aspectFlags, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::optional<VulkanTexture> texture;

    if (auto image = CreateImage(device, format, width, height, mipLevels, tiling, usageFlags, propertyFlags); image)
        if (auto view = CreateImageView(device, *image, aspectFlags); view)
            texture.emplace(*image, *view);

    /*auto sampler = CreateTextureSampler(app.vulkanDevice->handle(), image->mipLevels);

    if (!sampler)
        return { };*/

    return texture;
}
