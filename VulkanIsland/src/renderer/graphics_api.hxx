#pragma once

#include "main.hxx"
#include "graphics.hxx"


namespace graphics_api
{
    struct vulkan final {

        VkPrimitiveTopology constexpr operator() (graphics::PRIMITIVE_TOPOLOGY topology) const noexcept;

        VkCullModeFlags constexpr operator() (graphics::CULL_MODE cull_mode) const noexcept;

        VkPolygonMode constexpr operator() (graphics::POLYGON_MODE polygon_mode) const noexcept;

        VkFrontFace constexpr operator() (graphics::POLYGON_FRONT_FACE front_face) const noexcept;

        VkCompareOp constexpr operator() (graphics::COMPARE_OPERATION compare_operation) const noexcept;

        VkBlendFactor constexpr operator() (graphics::BLEND_FACTOR blend_factor) const noexcept;

        VkBlendOp constexpr operator() (graphics::BLEND_OPERATION blend_operation) const noexcept;

        VkColorComponentFlags constexpr operator() (graphics::COLOR_COMPONENT color_component) const noexcept;

        VkShaderStageFlagBits constexpr operator() (graphics::PIPELINE_SHADER_STAGE shader_stage) const noexcept;
    };
}
