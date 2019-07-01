#define _SCL_SECURE_NO_WARNINGS

#include "image.hxx"
#include "resource.hxx"
#include "commandBuffer.hxx"


[[nodiscard]] std::optional<VkFormat> FindDepthImageFormat(VulkanDevice const &device) noexcept
{
    auto format = FindSupportedImageFormat(
        device,
        std::array{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    return *format;
}


std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, VkFormat format, VkImageViewType type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels, VkSampleCountFlagBits samplesCount, VkImageTiling tiling,
              VkImageAspectFlags aspectFlags, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::optional<VulkanTexture> texture;

    if (auto image = device.resourceManager().CreateImage(format, width, height, mipLevels, samplesCount, tiling, usageFlags, propertyFlags); image)
        if (auto view = device.resourceManager().CreateImageView(*image, type, aspectFlags); view)
#if NOT_YET_IMPLEMENTED
            if (auto sampler = device.resourceManager().CreateImageSampler(image->mipLevels()); sampler)
                texture.emplace(image, *view, sampler);
#else
            texture.emplace(image, *view, nullptr);
#endif


    return texture;
}

