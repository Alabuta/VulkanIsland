#define _SCL_SECURE_NO_WARNINGS

#include "image.hxx"
#include "resource.hxx"
#include "commandBuffer.hxx"


[[nodiscard]] std::optional<graphics::FORMAT> FindDepthImageFormat(VulkanDevice const &device) noexcept
{
    auto format = FindSupportedImageFormat(
        device,
        { graphics::FORMAT::D32_SFLOAT, graphics::FORMAT::D32_SFLOAT_S8_UINT, graphics::FORMAT::D24_UNORM_S8_UINT },
        graphics::IMAGE_TILING::OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    return *format;
}


[[nodiscard]] std::optional<graphics::FORMAT>
FindSupportedImageFormat(VulkanDevice const &device, std::vector<graphics::FORMAT> const &candidates, graphics::IMAGE_TILING tiling, VkFormatFeatureFlags features) noexcept
{
    auto physicalDevice = device.physical_handle();

    auto it_format = std::find_if(std::cbegin(candidates), std::cend(candidates), [physicalDevice, tiling, features] (auto candidate)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, convert_to::vulkan(candidate), &properties);

        switch (tiling) {
            case graphics::IMAGE_TILING::LINEAR:
                return (properties.linearTilingFeatures & features) == features;

            case graphics::IMAGE_TILING::OPTIMAL:
                return (properties.optimalTilingFeatures & features) == features;

            default:
                return false;
        }
    });

    return it_format != std::cend(candidates) ? *it_format : std::optional<graphics::FORMAT>();
}

std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, graphics::FORMAT format, VkImageViewType type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels, std::uint32_t samplesCount, graphics::IMAGE_TILING tiling,
              VkImageAspectFlags aspectFlags, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags)
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

