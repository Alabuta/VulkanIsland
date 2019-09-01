#include "graphics_api.hxx"


namespace convert_to
{
    VkBool32 vulkan_api::operator() (bool boolean) const noexcept
    {
        return static_cast<VkBool32>(boolean);
    }

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

    VkShaderStageFlagBits vulkan_api::operator() (graphics::SHADER_STAGE shader_stage) const noexcept
    {
        switch (shader_stage) {
            case graphics::SHADER_STAGE::VERTEX:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

            case graphics::SHADER_STAGE::TESS_CONTROL:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

            case graphics::SHADER_STAGE::TESS_EVAL:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

            case graphics::SHADER_STAGE::GEOMETRY:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;

            case graphics::SHADER_STAGE::FRAGMENT:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

            case graphics::SHADER_STAGE::COMPUTE:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

            default:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    VkPipelineStageFlagBits vulkan_api::operator() (graphics::PIPELINE_STAGE pipeline_stage) const noexcept
    {
        switch (pipeline_stage) {
            case graphics::PIPELINE_STAGE::TOP_OF_PIPE:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            case graphics::PIPELINE_STAGE::DRAW_INDIRECT:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

            case graphics::PIPELINE_STAGE::VERTEX_INPUT:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

            case graphics::PIPELINE_STAGE::VERTEX_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

            case graphics::PIPELINE_STAGE::TESSELLATION_CONTROL_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;

            case graphics::PIPELINE_STAGE::TESSELLATION_EVALUATION_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

            case graphics::PIPELINE_STAGE::GEOMETRY_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

            case graphics::PIPELINE_STAGE::FRAGMENT_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            case graphics::PIPELINE_STAGE::EARLY_FRAGMENT_TESTS:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

            case graphics::PIPELINE_STAGE::LATE_FRAGMENT_TESTS:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            case graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            case graphics::PIPELINE_STAGE::COMPUTE_SHADER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            case graphics::PIPELINE_STAGE::TRANSFER:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;

            case graphics::PIPELINE_STAGE::BOTTOM_OF_PIPE:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

            case graphics::PIPELINE_STAGE::HOST:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT;

            case graphics::PIPELINE_STAGE::ALL_GRAPHICS:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

            case graphics::PIPELINE_STAGE::ALL_COMMANDS:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            default:
                return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
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

    VkStencilOp vulkan_api::operator() (graphics::STENCIL_OPERATION stencil_operation) const noexcept
    {
        switch (stencil_operation) {
            case graphics::STENCIL_OPERATION::KEEP:
                return VkStencilOp::VK_STENCIL_OP_KEEP;

            case graphics::STENCIL_OPERATION::ZERO:
                return VkStencilOp::VK_STENCIL_OP_ZERO;

            case graphics::STENCIL_OPERATION::REPLACE:
                return VkStencilOp::VK_STENCIL_OP_REPLACE;

            case graphics::STENCIL_OPERATION::INCREMENT_AND_CLAMP:
                return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;

            case graphics::STENCIL_OPERATION::DECREMENT_AND_CLAMP:
                return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;

            case graphics::STENCIL_OPERATION::INVERT:
                return VkStencilOp::VK_STENCIL_OP_INVERT;

            case graphics::STENCIL_OPERATION::INCREMENT_AND_WRAP:
                return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;

            case graphics::STENCIL_OPERATION::DECREMENT_AND_WRAP:
                return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;

            default:
                return VkStencilOp::VK_STENCIL_OP_MAX_ENUM;
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

            // Signed and usigned byte integer formats.
            case graphics::FORMAT::R8_SNORM:
                return VkFormat::VK_FORMAT_R8_SNORM;

            case graphics::FORMAT::RG8_SNORM:
                return VkFormat::VK_FORMAT_R8G8_SNORM;

            case graphics::FORMAT::RGB8_SNORM:
                return VkFormat::VK_FORMAT_R8G8B8_SNORM;

            case graphics::FORMAT::RGBA8_SNORM:
                return VkFormat::VK_FORMAT_R8G8B8A8_SNORM;

            case graphics::FORMAT::R8_UNORM:
                return VkFormat::VK_FORMAT_R8_UNORM;

            case graphics::FORMAT::RG8_UNORM:
                return VkFormat::VK_FORMAT_R8G8_UNORM;

            case graphics::FORMAT::RGB8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8_UNORM;

            case graphics::FORMAT::RGBA8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

            case graphics::FORMAT::R8_SINT:
                return VkFormat::VK_FORMAT_R8_SINT;

            case graphics::FORMAT::RG8_SINT:
                return VkFormat::VK_FORMAT_R8G8_SINT;

            case graphics::FORMAT::RGB8_SINT:
                return VkFormat::VK_FORMAT_R8G8B8_SINT;

            case graphics::FORMAT::RGBA8_SINT:
                return VkFormat::VK_FORMAT_R8G8B8A8_SINT;

            case graphics::FORMAT::R8_UINT:
                return VkFormat::VK_FORMAT_R8_UINT;

            case graphics::FORMAT::RG8_UINT:
                return VkFormat::VK_FORMAT_R8G8_UINT;

            case graphics::FORMAT::RGB8_UINT:
                return VkFormat::VK_FORMAT_R8G8B8_UINT;

            case graphics::FORMAT::RGBA8_UINT:
                return VkFormat::VK_FORMAT_R8G8B8A8_UINT;

            // Signed and unsigned two byte integer formats.
            case graphics::FORMAT::R16_SNORM:
                return VkFormat::VK_FORMAT_R16_SNORM;

            case graphics::FORMAT::RG16_SNORM:
                return VkFormat::VK_FORMAT_R16G16_SNORM;

            case graphics::FORMAT::RGB16_SNORM:
                return VkFormat::VK_FORMAT_R16G16B16_SNORM;

            case graphics::FORMAT::RGBA16_SNORM:
                return VkFormat::VK_FORMAT_R16G16B16A16_SNORM;

            case graphics::FORMAT::R16_UNORM:
                return VkFormat::VK_FORMAT_R16_UNORM;

            case graphics::FORMAT::RG16_UNORM:
                return VkFormat::VK_FORMAT_R16G16_UNORM;

            case graphics::FORMAT::RGB16_UNORM:
                return VkFormat::VK_FORMAT_R16G16B16_UNORM;

            case graphics::FORMAT::RGBA16_UNORM:
                return VkFormat::VK_FORMAT_R16G16B16A16_UNORM;

            case graphics::FORMAT::R16_SINT:
                return VkFormat::VK_FORMAT_R16_SINT;

            case graphics::FORMAT::RG16_SINT:
                return VkFormat::VK_FORMAT_R16G16_SINT;

            case graphics::FORMAT::RGB16_SINT:
                return VkFormat::VK_FORMAT_R16G16B16_SINT;

            case graphics::FORMAT::RGBA16_SINT:
                return VkFormat::VK_FORMAT_R16G16B16A16_SINT;

            case graphics::FORMAT::R16_UINT:
                return VkFormat::VK_FORMAT_R16_UINT;

            case graphics::FORMAT::RG16_UINT:
                return VkFormat::VK_FORMAT_R16G16_UINT;

            case graphics::FORMAT::RGB16_UINT:
                return VkFormat::VK_FORMAT_R16G16B16_UINT;

            case graphics::FORMAT::RGBA16_UINT:
                return VkFormat::VK_FORMAT_R16G16B16A16_UINT;

            // Signed and unsigned four byte integer formats.
            case graphics::FORMAT::R32_SINT:
                return VkFormat::VK_FORMAT_R32_SINT;

            case graphics::FORMAT::RG32_SINT:
                return VkFormat::VK_FORMAT_R32G32_SINT;

            case graphics::FORMAT::RGB32_SINT:
                return VkFormat::VK_FORMAT_R32G32B32_SINT;

            case graphics::FORMAT::RGBA32_SINT:
                return VkFormat::VK_FORMAT_R32G32B32A32_SINT;

            case graphics::FORMAT::R32_UINT:
                return VkFormat::VK_FORMAT_R32_UINT;

            case graphics::FORMAT::RG32_UINT:
                return VkFormat::VK_FORMAT_R32G32_UINT;

            case graphics::FORMAT::RGB32_UINT:
                return VkFormat::VK_FORMAT_R32G32B32_UINT;

            case graphics::FORMAT::RGBA32_UINT:
                return VkFormat::VK_FORMAT_R32G32B32A32_UINT;

            // Two byte float formats.
            case graphics::FORMAT::R16_SFLOAT:
                return VkFormat::VK_FORMAT_R16_SFLOAT;

            case graphics::FORMAT::RG16_SFLOAT:
                return VkFormat::VK_FORMAT_R16G16_SFLOAT;

            case graphics::FORMAT::RGB16_SFLOAT:
                return VkFormat::VK_FORMAT_R16G16B16_SFLOAT;

            case graphics::FORMAT::RGBA16_SFLOAT:
                return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;

            // Four byte float formats.
            case graphics::FORMAT::R32_SFLOAT:
                return VkFormat::VK_FORMAT_R32_SFLOAT;

            case graphics::FORMAT::RG32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32_SFLOAT;

            case graphics::FORMAT::RGB32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;

            case graphics::FORMAT::RGBA32_SFLOAT:
                return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;

            // Depth-stenci formats.
            case graphics::FORMAT::D24_UNORM_S8_UINT:
                return VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;

            case graphics::FORMAT::D32_SFLOAT:
                return VkFormat::VK_FORMAT_D32_SFLOAT;

            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
                return VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;

            // Special formats.
            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:
                return VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32;

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

    VkColorSpaceKHR vulkan_api::operator()(graphics::COLOR_SPACE color_space) const noexcept
    {
        switch (color_space) {
            case graphics::COLOR_SPACE::SRGB_NONLINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

            case graphics::COLOR_SPACE::EXTENDED_SRGB_LINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;

            case graphics::COLOR_SPACE::EXTENDED_SRGB_NONLINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;

            case graphics::COLOR_SPACE::BT709_LINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_BT709_LINEAR_EXT;

            case graphics::COLOR_SPACE::BT709_NONLINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_BT709_NONLINEAR_EXT;

            case graphics::COLOR_SPACE::ADOBE_RGB_LINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;

            case graphics::COLOR_SPACE::ADOBE_RGB_NONLINEAR:
                return VkColorSpaceKHR::VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;

            case graphics::COLOR_SPACE::HDR10_ST2084:
                return VkColorSpaceKHR::VK_COLOR_SPACE_HDR10_ST2084_EXT;

            case graphics::COLOR_SPACE::HDR10_HLG:
                return VkColorSpaceKHR::VK_COLOR_SPACE_HDR10_HLG_EXT;

            case graphics::COLOR_SPACE::PASS_THROUGH:
                return VkColorSpaceKHR::VK_COLOR_SPACE_PASS_THROUGH_EXT;

            default:
                return VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;
        }
    }
}