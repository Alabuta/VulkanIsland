#define _SCL_SECURE_NO_WARNINGS

#include "renderer/command_buffer.hxx"
#include "graphics/graphics_api.hxx"
#include "resource.hxx"
#include "image.hxx"


[[nodiscard]] std::optional<graphics::FORMAT> FindDepthImageFormat(vulkan::device const &device) noexcept
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
FindSupportedImageFormat(vulkan::device const &device, std::vector<graphics::FORMAT> const &candidates, graphics::IMAGE_TILING tiling, VkFormatFeatureFlags features) noexcept
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


[[nodiscard]] std::optional<graphics::FORMAT>
find_supported_image_format(vulkan::device const &device, std::vector<graphics::FORMAT> const &candidates,
                            graphics::IMAGE_TILING tiling, graphics::FORMAT_FEATURE features)
{
    auto it_format = std::find_if(std::cbegin(candidates), std::cend(candidates), [&device, tiling, features] (auto candidate)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device.physical_handle(), convert_to::vulkan(candidate), &properties);

        switch (tiling) {
            case graphics::IMAGE_TILING::LINEAR:
                return (properties.linearTilingFeatures & convert_to::vulkan(features)) == convert_to::vulkan(features);

            case graphics::IMAGE_TILING::OPTIMAL:
                return (properties.optimalTilingFeatures & convert_to::vulkan(features)) == convert_to::vulkan(features);

            default:
                return false;
        }
    });

    return it_format != std::cend(candidates) ? *it_format : std::optional<graphics::FORMAT>{ };
}

std::shared_ptr<resource::texture>
CreateTexture(vulkan::device const &device, ResourceManager &resource_manager, graphics::FORMAT format, graphics::IMAGE_VIEW_TYPE view_type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mip_levels, std::uint32_t samples_count, graphics::IMAGE_TILING tiling,
              VkImageAspectFlags aspectFlags, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::shared_ptr<resource::texture> texture;

    if (auto image = resource_manager.CreateImage(format, width, height, mip_levels, samples_count, tiling, usageFlags, propertyFlags); image)
        if (auto view = resource_manager.CreateImageView(image, view_type, aspectFlags); view)
#if NOT_YET_IMPLEMENTED
            if (auto sampler = resource_manager.CreateImageSampler(image->mip_levels()); sampler)
                texture.emplace(image, *view, sampler);
#else
            texture = std::make_shared<resource::texture>(image, view, nullptr);
#endif


    return texture;
}

