#pragma once

#include "main.hxx"
#include "graphics.hxx"


namespace convert_to
{
    struct vulkan_api final {

        VkPrimitiveTopology operator() (graphics::PRIMITIVE_TOPOLOGY topology) const noexcept;

        VkShaderStageFlagBits operator() (graphics::SHADER_STAGE shader_stage) const noexcept;

        VkPipelineStageFlagBits operator() (graphics::PIPELINE_STAGE pipeline_stage) const noexcept;

        VkCullModeFlags operator() (graphics::CULL_MODE cull_mode) const noexcept;

        VkPolygonMode operator() (graphics::POLYGON_MODE polygon_mode) const noexcept;

        VkFrontFace operator() (graphics::POLYGON_FRONT_FACE front_face) const noexcept;

        VkCompareOp operator() (graphics::COMPARE_OPERATION compare_operation) const noexcept;

        VkBlendFactor operator() (graphics::BLEND_FACTOR blend_factor) const noexcept;

        VkBlendOp operator() (graphics::BLEND_OPERATION blend_operation) const noexcept;

        VkColorComponentFlags operator() (graphics::COLOR_COMPONENT color_component) const noexcept;

        VkImageLayout operator() (graphics::IMAGE_LAYOUT image_layout) const noexcept;

        VkImageTiling operator() (graphics::IMAGE_TILING image_tiling) const noexcept;

        VkFormat operator() (graphics::FORMAT format) const noexcept;

        VkAttachmentLoadOp operator() (graphics::ATTACHMENT_LOAD_TREATMENT load_treatment) const noexcept;

        VkAttachmentStoreOp operator() (graphics::ATTACHMENT_STORE_TREATMENT store_treatment) const noexcept;
    };

    vulkan_api constexpr vulkan;
}
