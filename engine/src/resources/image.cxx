/*#ifdef _MSC_VER
    #define _SCL_SECURE_NO_WARNINGS
#endif*/

#include <ranges>

#include "renderer/command_buffer.hxx"
#include "graphics/graphics_api.hxx"
#include "image.hxx"


[[nodiscard]] std::optional<graphics::FORMAT>
find_supported_image_format(vulkan::device const &device, std::vector<graphics::FORMAT> const &candidates,
                            graphics::IMAGE_TILING tiling, graphics::FORMAT_FEATURE features)
{
    auto it_format = std::ranges::find_if(candidates, [&device, tiling, features] (auto candidate)
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
