#include "graphics_api.hxx"


namespace convert_to
{
    VkPrimitiveTopology vulkan_api::operator() (graphics::PRIMITIVE_TOPOLOGY topology) const noexcept
    {
        switch (topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

            case graphics::PRIMITIVE_TOPOLOGY::LINES:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

            case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_FAN:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

            default:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    VkCullModeFlags vulkan_api::operator() (graphics::CULL_MODE cull_mode) const noexcept
    {
        switch (cull_mode) {
            case graphics::CULL_MODE::NONE:
                return VkCullModeFlagBits::VK_CULL_MODE_NONE;

            case graphics::CULL_MODE::FRONT:
                return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;

            case graphics::CULL_MODE::BACK:
                return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;

            case graphics::CULL_MODE::FRONT_AND_BACK:
                return VkCullModeFlagBits::VK_CULL_MODE_FRONT_AND_BACK;

            default:
                return VkCullModeFlagBits::VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        }
    }

    VkPolygonMode vulkan_api::operator() (graphics::POLYGON_MODE polygon_mode) const noexcept
    {
        switch (polygon_mode) {
            case graphics::POLYGON_MODE::FILL:
                return VkPolygonMode::VK_POLYGON_MODE_FILL;

            case graphics::POLYGON_MODE::LINE:
                return VkPolygonMode::VK_POLYGON_MODE_LINE;

            case graphics::POLYGON_MODE::POINT:
                return VkPolygonMode::VK_POLYGON_MODE_POINT;

            default:
                return VkPolygonMode::VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    VkFrontFace vulkan_api::operator() (graphics::POLYGON_FRONT_FACE front_face) const noexcept
    {
        switch (front_face) {
            case graphics::POLYGON_FRONT_FACE::COUNTER_CLOCKWISE:
                return VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;

            case graphics::POLYGON_FRONT_FACE::CLOCKWISE:
                return VkFrontFace::VK_FRONT_FACE_CLOCKWISE;

            default:
                return VkFrontFace::VK_FRONT_FACE_MAX_ENUM;
        }
    }

    VkCompareOp vulkan_api::operator() (graphics::COMPARE_OPERATION compare_operation) const noexcept
    {
        switch (compare_operation) {
            case graphics::COMPARE_OPERATION::NEVER:
                return VkCompareOp::VK_COMPARE_OP_NEVER;

            case graphics::COMPARE_OPERATION::LESS:
                return VkCompareOp::VK_COMPARE_OP_LESS;

            case graphics::COMPARE_OPERATION::EQUAL:
                return VkCompareOp::VK_COMPARE_OP_EQUAL;

            case graphics::COMPARE_OPERATION::LESS_OR_EQUAL:
                return VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;

            case graphics::COMPARE_OPERATION::GREATER:
                return VkCompareOp::VK_COMPARE_OP_GREATER;

            case graphics::COMPARE_OPERATION::NOT_EQUAL:
                return VkCompareOp::VK_COMPARE_OP_NOT_EQUAL;

            case graphics::COMPARE_OPERATION::GREATER_OR_EQUAL:
                return VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;

            case graphics::COMPARE_OPERATION::ALWAYS:
                return VkCompareOp::VK_COMPARE_OP_ALWAYS;

            default:
                return VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
        }
    }

    VkBlendFactor vulkan_api::operator() (graphics::BLEND_FACTOR blend_factor) const noexcept
    {
        switch (blend_factor) {
            case graphics::BLEND_FACTOR::ZERO:
                return VkBlendFactor::VK_BLEND_FACTOR_ZERO;

            case graphics::BLEND_FACTOR::ONE:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE;

            case graphics::BLEND_FACTOR::SRC_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;

            case graphics::BLEND_FACTOR::ONE_MINUS_SRC_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

            case graphics::BLEND_FACTOR::DST_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;

            case graphics::BLEND_FACTOR::ONE_MINUS_DST_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

            case graphics::BLEND_FACTOR::SRC_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;

            case graphics::BLEND_FACTOR::ONE_MINUS_SRC_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

            case graphics::BLEND_FACTOR::DST_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;

            case graphics::BLEND_FACTOR::ONE_MINUS_DST_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

            case graphics::BLEND_FACTOR::CONSTANT_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;

            case graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

            case graphics::BLEND_FACTOR::CONSTANT_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;

            case graphics::BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;

            case graphics::BLEND_FACTOR::SRC_ALPHA_SATURATE:
                return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

            case graphics::BLEND_FACTOR::SRC1_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

            case graphics::BLEND_FACTOR::ONE_MINUS_SRC1_COLOR:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

            case graphics::BLEND_FACTOR::SRC1_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

            case graphics::BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA:
                return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

            default:
                return VkBlendFactor::VK_BLEND_FACTOR_MAX_ENUM;

        }
    }

    VkBlendOp vulkan_api::operator() (graphics::BLEND_OPERATION blend_operation) const noexcept
    {
        switch (blend_operation) {
            case graphics::BLEND_OPERATION::ADD:
                return VkBlendOp::VK_BLEND_OP_ADD;

            case graphics::BLEND_OPERATION::SUBTRACT:
                return VkBlendOp::VK_BLEND_OP_SUBTRACT;

            default:
                return VkBlendOp::VK_BLEND_OP_MAX_ENUM;
        }
    }

    VkColorComponentFlags vulkan_api::operator() (graphics::COLOR_COMPONENT color_component) const noexcept
    {
        switch (color_component) {
            case graphics::COLOR_COMPONENT::R:
                return VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT;

            case graphics::COLOR_COMPONENT::G:
                return VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT;

            case graphics::COLOR_COMPONENT::B:
                return VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT;

            case graphics::COLOR_COMPONENT::A:
                return VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

            case graphics::COLOR_COMPONENT::RGB:
                return
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT |
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT;

            case graphics::COLOR_COMPONENT::RGBA:
                return
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT |
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT |
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

            default:
                return
                    VkColorComponentFlagBits::VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
        }
    }

    VkShaderStageFlagBits vulkan_api::operator() (graphics::PIPELINE_SHADER_STAGE shader_stage) const noexcept
    {
        switch (shader_stage) {
            case graphics::PIPELINE_SHADER_STAGE::VERTEX:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

            case graphics::PIPELINE_SHADER_STAGE::TESS_CONTROL:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

            case graphics::PIPELINE_SHADER_STAGE::TESS_EVAL:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

            case graphics::PIPELINE_SHADER_STAGE::GEOMETRY:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;

            case graphics::PIPELINE_SHADER_STAGE::FRAGMENT:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

            case graphics::PIPELINE_SHADER_STAGE::COMPUTE:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

            default:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    VkImageLayout vulkan_api::operator() (graphics::IMAGE_LAYOUT image_layout) const noexcept
    {
        switch (image_layout) {
            case graphics::IMAGE_LAYOUT::UNDEFINED:
                return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

            case graphics::IMAGE_LAYOUT::GENERAL:
                return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;

            case graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT:
                return VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            case graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT:
                return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            case graphics::IMAGE_LAYOUT::DEPTH_STENCIL_READ_ONLY:
                return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            case graphics::IMAGE_LAYOUT::SHADER_READ_ONLY:
                return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            case graphics::IMAGE_LAYOUT::TRANSFER_SOURCE:
                return VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            case graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION:
                return VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            case graphics::IMAGE_LAYOUT::PREINITIALIZED:
                return VkImageLayout::VK_IMAGE_LAYOUT_PREINITIALIZED;

            case graphics::IMAGE_LAYOUT::DEPTH_READ_ONLY_STENCIL_ATTACHMENT:
                return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;

            case graphics::IMAGE_LAYOUT::DEPTH_ATTACHMENT_STENCIL_READ_ONLY:
                return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

            case graphics::IMAGE_LAYOUT::PRESENT_SOURCE:
                return VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            default:
                return VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM;
        }
    }

    VkImageTiling vulkan_api::operator() (graphics::IMAGE_TILING image_tiling) const noexcept
    {
        switch (image_tiling) {
            case graphics::IMAGE_TILING::OPTIMAL:
                return VkImageTiling::VK_IMAGE_TILING_OPTIMAL;

            case graphics::IMAGE_TILING::LINEAR:
                return VkImageTiling::VK_IMAGE_TILING_LINEAR;

            default:
                return VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
        }
    }

    VkFormat vulkan_api::operator() (graphics::FORMAT format) const noexcept
    {
        switch (format) {
            case graphics::FORMAT::UNDEFINED:
                return VkFormat::VK_FORMAT_UNDEFINED;

            case graphics::FORMAT::RGB8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8_UNORM;

            case graphics::FORMAT::RGBA8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:
                return VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case graphics::FORMAT::R16_SFLOAT:
                return VkFormat::VK_FORMAT_R16_SFLOAT;

            case graphics::FORMAT::RG16_SFLOAT:
                return VkFormat::VK_FORMAT_R16G16_SFLOAT;

            case graphics::FORMAT::RGBA16_SFLOAT:
                return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;

            case graphics::FORMAT::R32_SFLOAT:
                return VkFormat::VK_FORMAT_R32_SFLOAT;

            case graphics::FORMAT::RG32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32_SFLOAT;

            case graphics::FORMAT::RGB32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;

            case graphics::FORMAT::RGBA32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;

            case graphics::FORMAT::D24_UNORM_S8_UINT:
                return VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;

            case graphics::FORMAT::D32_SFLOAT:
                return VkFormat::VK_FORMAT_D32_SFLOAT;

            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
                return VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;

            default:
                return VkFormat::VK_FORMAT_MAX_ENUM;
        }
    }

    VkAttachmentLoadOp vulkan_api::operator() (graphics::ATTACHMENT_LOAD_TREATMENT load_treatment) const noexcept
    {
        switch (load_treatment) {
            case graphics::ATTACHMENT_LOAD_TREATMENT::LOAD:
                return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

            case graphics::ATTACHMENT_LOAD_TREATMENT::CLEAR:
                return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;

            case graphics::ATTACHMENT_LOAD_TREATMENT::DONT_CARE:
                return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;

            default:
                return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }

    VkAttachmentStoreOp vulkan_api::operator() (graphics::ATTACHMENT_STORE_TREATMENT store_treatment) const noexcept
    {
        switch (store_treatment) {
            case graphics::ATTACHMENT_STORE_TREATMENT::STORE:
                return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

            case graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE:
                return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;

            default:
                return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }
}