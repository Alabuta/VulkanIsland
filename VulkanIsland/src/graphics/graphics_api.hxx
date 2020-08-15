#pragma once


#include <volk/volk.h>

#include "graphics.hxx"


namespace convert_to
{
    VkBool32 vulkan(bool boolean) noexcept;

    VkPrimitiveTopology vulkan(graphics::PRIMITIVE_TOPOLOGY topology) noexcept;

    VkVertexInputRate vulkan(graphics::VERTEX_INPUT_RATE input_rate) noexcept;

    VkShaderStageFlagBits vulkan(graphics::SHADER_STAGE shader_stage) noexcept;

    VkPipelineStageFlags vulkan(graphics::PIPELINE_STAGE pipeline_stage) noexcept;

    VkDescriptorType vulkan(graphics::DESCRIPTOR_TYPE descriptor_type) noexcept;

    VkCullModeFlags vulkan(graphics::CULL_MODE cull_mode) noexcept;

    VkPolygonMode vulkan(graphics::POLYGON_MODE polygon_mode) noexcept;

    VkFrontFace vulkan(graphics::POLYGON_FRONT_FACE front_face) noexcept;

    VkCompareOp vulkan(graphics::COMPARE_OPERATION compare_operation) noexcept;

    VkStencilOp vulkan(graphics::STENCIL_OPERATION stencil_operation) noexcept;

    VkBlendFactor vulkan(graphics::BLEND_FACTOR blend_factor) noexcept;

    VkBlendOp vulkan(graphics::BLEND_OPERATION blend_operation) noexcept;

    VkLogicOp vulkan(graphics::LOGIC_OPERATION logic_operation) noexcept;

    VkColorComponentFlags vulkan(graphics::COLOR_COMPONENT color_component) noexcept;

    VkImageLayout vulkan(graphics::IMAGE_LAYOUT image_layout) noexcept;

    VkImageTiling vulkan(graphics::IMAGE_TILING image_tiling) noexcept;

    VkImageType vulkan(graphics::IMAGE_TYPE image_type) noexcept;

    VkImageViewType vulkan(graphics::IMAGE_VIEW_TYPE image_view_type) noexcept;

    VkFilter vulkan(graphics::TEXTURE_FILTER texture_filter) noexcept;

    VkSamplerMipmapMode  vulkan(graphics::TEXTURE_MIPMAP_MODE texture_mipmap_mode) noexcept;

    VkMemoryPropertyFlags vulkan(graphics::MEMORY_PROPERTY_TYPE memory_property_type) noexcept;

    VkFormat vulkan(graphics::FORMAT format) noexcept;

    VkFormatFeatureFlags vulkan(graphics::FORMAT_FEATURE format_feature) noexcept;

    VkAttachmentLoadOp vulkan(graphics::ATTACHMENT_LOAD_TREATMENT load_treatment) noexcept;

    VkAttachmentStoreOp vulkan(graphics::ATTACHMENT_STORE_TREATMENT store_treatment) noexcept;

    VkColorSpaceKHR vulkan(graphics::COLOR_SPACE color_space) noexcept;

    VkSampleCountFlagBits vulkan(std::uint32_t samples_count) noexcept;

    VkBufferUsageFlags vulkan(graphics::BUFFER_USAGE buffer_usage) noexcept;

    VkImageUsageFlags vulkan(graphics::IMAGE_USAGE image_usage) noexcept;

    VkQueueFlagBits vulkan(graphics::QUEUE_CAPABILITY queue_capability) noexcept;

    VkPresentModeKHR vulkan(graphics::PRESENTATION_MODE presentation_mode) noexcept;

    VkImageAspectFlags vulkan(graphics::IMAGE_ASPECT image_aspect) noexcept;

    VkAccessFlags vulkan(graphics::MEMORY_ACCESS_TYPE access_type) noexcept;

    VkSharingMode vulkan(graphics::RESOURCE_SHARING_MODE sharing_mode) noexcept;
}
