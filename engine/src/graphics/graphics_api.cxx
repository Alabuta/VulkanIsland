#include "graphics_api.hxx"


namespace convert_to
{
    VkBool32 vulkan(bool boolean) noexcept
    {
        return static_cast<VkBool32>(boolean);
    }

    VkPrimitiveTopology vulkan(graphics::PRIMITIVE_TOPOLOGY topology) noexcept
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

            case graphics::PRIMITIVE_TOPOLOGY::LINE_LIST_WITH_ADJACENCY:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;

            case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP_WITH_ADJACENCY:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_LIST_WITH_ADJACENCY:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP_WITH_ADJACENCY:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;

            default:
                return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    VkVertexInputRate vulkan(graphics::VERTEX_INPUT_RATE input_rate) noexcept
    {
        switch (input_rate) {
            case graphics::VERTEX_INPUT_RATE::PER_VERTEX:
                return VK_VERTEX_INPUT_RATE_VERTEX;

            case graphics::VERTEX_INPUT_RATE::PER_INSTANCE:
                return VK_VERTEX_INPUT_RATE_INSTANCE;

            default:
                return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }

    VkShaderStageFlagBits vulkan(graphics::SHADER_STAGE shader_stage) noexcept
    {
        VkShaderStageFlags result = 0;

        using E = std::underlying_type_t<graphics::SHADER_STAGE>;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::VERTEX))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::TESS_CONTROL))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::TESS_EVAL))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::GEOMETRY))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::FRAGMENT))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

        if (static_cast<E>(shader_stage & graphics::SHADER_STAGE::COMPUTE))
            result |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

        return static_cast<VkShaderStageFlagBits>(result);
    }

    VkPipelineStageFlags vulkan(graphics::PIPELINE_STAGE pipeline_stage) noexcept
    {
        VkPipelineStageFlags result = 0;

        using E = std::underlying_type_t<graphics::PIPELINE_STAGE>;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::TOP_OF_PIPE))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::DRAW_INDIRECT))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::VERTEX_INPUT))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::VERTEX_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::TESSELLATION_CONTROL_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::TESSELLATION_EVALUATION_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::GEOMETRY_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::FRAGMENT_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::EARLY_FRAGMENT_TESTS))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::LATE_FRAGMENT_TESTS))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::COMPUTE_SHADER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::TRANSFER))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::BOTTOM_OF_PIPE))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::HOST))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::ALL_GRAPHICS_PIPELINE_STAGES))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

        if (static_cast<E>(pipeline_stage & graphics::PIPELINE_STAGE::ALL_COMMANDS))
            result |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        return result;
    }

    VkDescriptorType vulkan(graphics::DESCRIPTOR_TYPE descriptor_type) noexcept
    {
        switch (descriptor_type) {
            case graphics::DESCRIPTOR_TYPE::SAMPLER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;

            case graphics::DESCRIPTOR_TYPE::COMBINED_IMAGE_SAMPLER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            case graphics::DESCRIPTOR_TYPE::SAMPLED_IMAGE:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            case graphics::DESCRIPTOR_TYPE::STORAGE_IMAGE:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            case graphics::DESCRIPTOR_TYPE::UNIFORM_TEXEL_BUFFER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

            case graphics::DESCRIPTOR_TYPE::STORAGE_TEXEL_BUFFER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

            case graphics::DESCRIPTOR_TYPE::UNIFORM_BUFFER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            case graphics::DESCRIPTOR_TYPE::STORAGE_BUFFER:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            case graphics::DESCRIPTOR_TYPE::UNIFORM_BUFFER_DYNAMIC:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

            case graphics::DESCRIPTOR_TYPE::STORAGE_BUFFER_DYNAMIC:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

            case graphics::DESCRIPTOR_TYPE::INPUT_ATTACHMENT:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

            default:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    VkCullModeFlags vulkan(graphics::CULL_MODE cull_mode) noexcept
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

    VkPolygonMode vulkan(graphics::POLYGON_MODE polygon_mode) noexcept
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

    VkFrontFace vulkan(graphics::POLYGON_FRONT_FACE front_face) noexcept
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

    VkCompareOp vulkan(graphics::COMPARE_OPERATION compare_operation) noexcept
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

    VkStencilOp vulkan(graphics::STENCIL_OPERATION stencil_operation) noexcept
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

    VkBlendFactor vulkan(graphics::BLEND_FACTOR blend_factor) noexcept
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

    VkBlendOp vulkan(graphics::BLEND_OPERATION blend_operation) noexcept
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

    VkLogicOp vulkan(graphics::LOGIC_OPERATION logic_operation) noexcept
    {
        switch (logic_operation) {
            case graphics::LOGIC_OPERATION::CLEAR:
                return VkLogicOp::VK_LOGIC_OP_CLEAR;

            case graphics::LOGIC_OPERATION::AND:
                return VkLogicOp::VK_LOGIC_OP_AND;

            case graphics::LOGIC_OPERATION::AND_REVERSE:
                return VkLogicOp::VK_LOGIC_OP_AND_REVERSE;

            case graphics::LOGIC_OPERATION::COPY:
                return VkLogicOp::VK_LOGIC_OP_COPY;

            case graphics::LOGIC_OPERATION::AND_INVERTED:
                return VkLogicOp::VK_LOGIC_OP_AND_INVERTED;

            case graphics::LOGIC_OPERATION::NO_OP:
                return VkLogicOp::VK_LOGIC_OP_NO_OP;

            case graphics::LOGIC_OPERATION::XOR:
                return VkLogicOp::VK_LOGIC_OP_XOR;

            case graphics::LOGIC_OPERATION::OR:
                return VkLogicOp::VK_LOGIC_OP_OR;

            case graphics::LOGIC_OPERATION::NOR:
                return VkLogicOp::VK_LOGIC_OP_NOR;

            case graphics::LOGIC_OPERATION::EQUIVALENT:
                return VkLogicOp::VK_LOGIC_OP_EQUIVALENT;

            case graphics::LOGIC_OPERATION::INVERT:
                return VkLogicOp::VK_LOGIC_OP_INVERT;

            case graphics::LOGIC_OPERATION::OR_REVERSE:
                return VkLogicOp::VK_LOGIC_OP_OR_REVERSE;

            case graphics::LOGIC_OPERATION::COPY_INVERTED:
                return VkLogicOp::VK_LOGIC_OP_COPY_INVERTED;

            case graphics::LOGIC_OPERATION::OR_INVERTED:
                return VkLogicOp::VK_LOGIC_OP_OR_INVERTED;

            case graphics::LOGIC_OPERATION::NAND:
                return VkLogicOp::VK_LOGIC_OP_NAND;

            case graphics::LOGIC_OPERATION::SET:
                return VkLogicOp::VK_LOGIC_OP_SET;

            default:
                return VkLogicOp::VK_LOGIC_OP_MAX_ENUM;
        }
    }

    VkColorComponentFlags vulkan(graphics::COLOR_COMPONENT color_component) noexcept
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

    VkImageLayout vulkan(graphics::IMAGE_LAYOUT image_layout) noexcept
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

    VkImageType vulkan(graphics::IMAGE_TYPE image_type) noexcept
    {
        switch (image_type) {
            case graphics::IMAGE_TYPE::TYPE_1D:
                return VkImageType::VK_IMAGE_TYPE_1D;

            case graphics::IMAGE_TYPE::TYPE_2D:
                return VkImageType::VK_IMAGE_TYPE_2D;

            case graphics::IMAGE_TYPE::TYPE_3D:
                return VkImageType::VK_IMAGE_TYPE_3D;

            default:
                return VkImageType::VK_IMAGE_TYPE_MAX_ENUM;
        }
    }

    VkImageViewType vulkan(graphics::IMAGE_VIEW_TYPE image_view_type) noexcept
    {
        switch (image_view_type) {
            case graphics::IMAGE_VIEW_TYPE::TYPE_1D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_1D;

            case graphics::IMAGE_VIEW_TYPE::TYPE_2D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

            case graphics::IMAGE_VIEW_TYPE::TYPE_3D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_3D;

            case graphics::IMAGE_VIEW_TYPE::TYPE_CUBE:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;

            case graphics::IMAGE_VIEW_TYPE::TYPE_1D_ARRAY:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_1D_ARRAY;

            case graphics::IMAGE_VIEW_TYPE::TYPE_2D_ARRAY:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;

            case graphics::IMAGE_VIEW_TYPE::TYPE_CUBE_ARRAY:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

            default:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    VkImageTiling vulkan(graphics::IMAGE_TILING image_tiling) noexcept
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

    VkFilter vulkan(graphics::TEXTURE_FILTER texture_filter) noexcept
    {
        switch (texture_filter) {
            case graphics::TEXTURE_FILTER::NEAREST:
                return VkFilter::VK_FILTER_NEAREST;

            case graphics::TEXTURE_FILTER::LINEAR:
                return VkFilter::VK_FILTER_LINEAR;

            case graphics::TEXTURE_FILTER::CUBIC:
                return VkFilter::VK_FILTER_CUBIC_IMG;

            default:
                return VkFilter::VK_FILTER_MAX_ENUM;
        }
    }

    VkSamplerMipmapMode vulkan(graphics::TEXTURE_MIPMAP_MODE texture_mipmap_mode) noexcept
    {
        switch (texture_mipmap_mode) {
            case graphics::TEXTURE_MIPMAP_MODE::NEAREST:
                return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;

            case graphics::TEXTURE_MIPMAP_MODE::LINEAR:
                return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;

            default:
                return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
        }
    }

    VkMemoryPropertyFlags vulkan(graphics::MEMORY_PROPERTY_TYPE memory_property_type) noexcept
    {
        VkMemoryPropertyFlags result = 0;

        using E = std::underlying_type_t<graphics::MEMORY_PROPERTY_TYPE>;

        if (static_cast<E>(memory_property_type & graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL))
            result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        if (static_cast<E>(memory_property_type & graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE))
            result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        if (static_cast<E>(memory_property_type & graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT))
            result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        if (static_cast<E>(memory_property_type & graphics::MEMORY_PROPERTY_TYPE::HOST_CACHED))
            result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        if (static_cast<E>(memory_property_type & graphics::MEMORY_PROPERTY_TYPE::LAZILY_ALLOCATED))
            result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

        return result;
    }

    VkFormat vulkan(graphics::FORMAT format) noexcept
    {
        switch (format) {
            case graphics::FORMAT::UNDEFINED:
                return VkFormat::VK_FORMAT_UNDEFINED;

            case graphics::FORMAT::RG4_UNORM_PACK8:
                return VkFormat::VK_FORMAT_R4G4_UNORM_PACK8;

            case graphics::FORMAT::R8_USCALED:
                return VkFormat::VK_FORMAT_R8_USCALED;

            // Signed and usigned byte integer formats.
            case graphics::FORMAT::R8_SNORM:
                return VkFormat::VK_FORMAT_R8_SNORM;

            case graphics::FORMAT::R8_SRGB:
                return VkFormat::VK_FORMAT_R8_SRGB;

            case graphics::FORMAT::R8_SSCALED:
                return VkFormat::VK_FORMAT_R8_SSCALED;

            case graphics::FORMAT::RG8_USCALED:
                return VkFormat::VK_FORMAT_R8G8_USCALED;

            case graphics::FORMAT::RG8_SSCALED:
                return VkFormat::VK_FORMAT_R8G8_SSCALED;

            case graphics::FORMAT::RG8_SNORM:
                return VkFormat::VK_FORMAT_R8G8_SNORM;

            case graphics::FORMAT::RG8_SRGB:
                return VkFormat::VK_FORMAT_R8G8_SRGB;

            case graphics::FORMAT::RGB8_SNORM:
                return VkFormat::VK_FORMAT_R8G8B8_SNORM;

            case graphics::FORMAT::RGB8_SRGB:
                return VkFormat::VK_FORMAT_R8G8B8_SRGB;

            case graphics::FORMAT::RGBA8_SNORM:
                return VkFormat::VK_FORMAT_R8G8B8A8_SNORM;

            case graphics::FORMAT::BGRA8_SNORM:
                return VkFormat::VK_FORMAT_B8G8R8A8_SNORM;

            case graphics::FORMAT::R8_UNORM:
                return VkFormat::VK_FORMAT_R8_UNORM;

            case graphics::FORMAT::RG8_UNORM:
                return VkFormat::VK_FORMAT_R8G8_UNORM;

            case graphics::FORMAT::RGB8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8_UNORM;

            case graphics::FORMAT::RGB8_USCALED:
                return VkFormat::VK_FORMAT_R8G8B8_USCALED;

            case graphics::FORMAT::RGB8_SSCALED:
                return VkFormat::VK_FORMAT_R8G8B8_SSCALED;

            case graphics::FORMAT::BGR8_UNORM:
                return VkFormat::VK_FORMAT_B8G8R8_UNORM;

            case graphics::FORMAT::BGR8_SNORM:
                return VkFormat::VK_FORMAT_B8G8R8_SNORM;

            case graphics::FORMAT::BGR8_USCALED:
                return VkFormat::VK_FORMAT_B8G8R8_USCALED;

            case graphics::FORMAT::BGR8_SSCALED:
                return VkFormat::VK_FORMAT_B8G8R8_SSCALED;

            case graphics::FORMAT::BGR8_UINT:
                return VkFormat::VK_FORMAT_B8G8R8_UINT;

            case graphics::FORMAT::BGR8_SINT:
                return VkFormat::VK_FORMAT_B8G8R8_SINT;

            case graphics::FORMAT::BGR8_SRGB:
                return VkFormat::VK_FORMAT_B8G8R8_SRGB;

            case graphics::FORMAT::RGBA8_UNORM:
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

            case graphics::FORMAT::RGBA8_USCALED:
                return VkFormat::VK_FORMAT_R8G8B8A8_USCALED;

            case graphics::FORMAT::RGBA8_SSCALED:
                return VkFormat::VK_FORMAT_R8G8B8A8_SSCALED;

            case graphics::FORMAT::BGRA8_USCALED:
                return VkFormat::VK_FORMAT_B8G8R8A8_USCALED;

            case graphics::FORMAT::BGRA8_UNORM:
                return VkFormat::VK_FORMAT_B8G8R8A8_UNORM;

            case graphics::FORMAT::BGRA8_SSCALED:
                return VkFormat::VK_FORMAT_B8G8R8A8_SSCALED;

            case graphics::FORMAT::RGBA8_SRGB:
                return VkFormat::VK_FORMAT_R8G8B8A8_SRGB;

            case graphics::FORMAT::BGRA8_SRGB:
                return VkFormat::VK_FORMAT_B8G8R8A8_SRGB;

            case graphics::FORMAT::ABGR8_SINT_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_SINT_PACK32;

            case graphics::FORMAT::A2RGB10_UINT_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_UINT_PACK32;

            case graphics::FORMAT::A2BGR10_UINT_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_UINT_PACK32;

            case graphics::FORMAT::ABGR8_USCALED_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_USCALED_PACK32;

            case graphics::FORMAT::ABGR8_SSCALED_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_SSCALED_PACK32;

            case graphics::FORMAT::A2RGB10_SNORM_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_SNORM_PACK32;

            case graphics::FORMAT::A2RGB10_USCALED_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_USCALED_PACK32;

            case graphics::FORMAT::A2RGB10_SSCALED_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_SSCALED_PACK32;

            case graphics::FORMAT::A2RGB10_SINT_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_SINT_PACK32;

            case graphics::FORMAT::A2BGR10_SNORM_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_SNORM_PACK32;

            case graphics::FORMAT::A2BGR10_USCALED_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_USCALED_PACK32;

            case graphics::FORMAT::A2BGR10_SSCALED_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_SSCALED_PACK32;

            case graphics::FORMAT::A2BGR10_SINT_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_SINT_PACK32;

            case graphics::FORMAT::ABGR8_UINT_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_UINT_PACK32;

            case graphics::FORMAT::R5G6B5_UNORM_PACK16:
                return VkFormat::VK_FORMAT_R5G6B5_UNORM_PACK16;

            case graphics::FORMAT::ABGR8_UNORM_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_UNORM_PACK32;

            case graphics::FORMAT::ABGR8_SNORM_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_SNORM_PACK32;

            case graphics::FORMAT::ABGR8_SRGB_PACK32:
                return VkFormat::VK_FORMAT_A8B8G8R8_SRGB_PACK32;

            case graphics::FORMAT::A2RGB10_UNORM_PACK32:
                return VkFormat::VK_FORMAT_A2R10G10B10_UNORM_PACK32;

            case graphics::FORMAT::A2BGR10_UNORM_PACK32:
                return VkFormat::VK_FORMAT_A2B10G10R10_UNORM_PACK32;

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

            case graphics::FORMAT::R16_USCALED:
                return VkFormat::VK_FORMAT_R16_USCALED;

            case graphics::FORMAT::R16_SSCALED:
                return VkFormat::VK_FORMAT_R16_SSCALED;

            case graphics::FORMAT::RGB16_SNORM:
                return VkFormat::VK_FORMAT_R16G16B16_SNORM;

            case graphics::FORMAT::RGBA16_SNORM:
                return VkFormat::VK_FORMAT_R16G16B16A16_SNORM;

            case graphics::FORMAT::R16_UNORM:
                return VkFormat::VK_FORMAT_R16_UNORM;

            case graphics::FORMAT::RG16_UNORM:
                return VkFormat::VK_FORMAT_R16G16_UNORM;

            case graphics::FORMAT::RG16_USCALED:
                return VkFormat::VK_FORMAT_R16G16_USCALED;

            case graphics::FORMAT::RG16_SSCALED:
                return VkFormat::VK_FORMAT_R16G16_SSCALED;

            case graphics::FORMAT::RGB16_USCALED:
                return VkFormat::VK_FORMAT_R16G16B16_USCALED;

            case graphics::FORMAT::RGB16_SSCALED:
                return VkFormat::VK_FORMAT_R16G16B16_SSCALED;

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

            case graphics::FORMAT::RGBA16_USCALED:
                return VkFormat::VK_FORMAT_R16G16B16A16_USCALED;

            case graphics::FORMAT::RGBA16_SSCALED:
                return VkFormat::VK_FORMAT_R16G16B16A16_SSCALED;

            case graphics::FORMAT::R16_UINT:
                return VkFormat::VK_FORMAT_R16_UINT;

            case graphics::FORMAT::RG16_UINT:
                return VkFormat::VK_FORMAT_R16G16_UINT;

            case graphics::FORMAT::RGB16_UINT:
                return VkFormat::VK_FORMAT_R16G16B16_UINT;

            case graphics::FORMAT::RGBA16_UINT:
                return VkFormat::VK_FORMAT_R16G16B16A16_UINT;

            case graphics::FORMAT::B5G6R5_UNORM_PACK16:
                return VkFormat::VK_FORMAT_B5G6R5_UNORM_PACK16;

            case graphics::FORMAT::A1RGB5_UNORM_PACK16:
                return VkFormat::VK_FORMAT_A1R5G5B5_UNORM_PACK16;

            case graphics::FORMAT::RGBA4_UNORM_PACK16:
                return VkFormat::VK_FORMAT_R4G4B4A4_UNORM_PACK16;

            case graphics::FORMAT::BGRA4_UNORM_PACK16:
                return VkFormat::VK_FORMAT_B4G4R4A4_UNORM_PACK16;

            case graphics::FORMAT::RGB5A1_UNORM_PACK16:
                return VkFormat::VK_FORMAT_R5G5B5A1_UNORM_PACK16;

            case graphics::FORMAT::BGR5A1_UNORM_PACK16:
                return VkFormat::VK_FORMAT_B5G5R5A1_UNORM_PACK16;

            case graphics::FORMAT::BGRA8_UINT:
                return VkFormat::VK_FORMAT_B8G8R8A8_UINT;

            case graphics::FORMAT::BGRA8_SINT:
                return VkFormat::VK_FORMAT_B8G8R8A8_SINT;

            case graphics::FORMAT::BGRG8_422_UNORM:
                return VkFormat::VK_FORMAT_B8G8R8G8_422_UNORM;

            case graphics::FORMAT::GBGR10X6_422_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;

            case graphics::FORMAT::BGRG10X6_422_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;

            case graphics::FORMAT::GBGR12X4_422_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;

            case graphics::FORMAT::BGRG12X4_422_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;

            case graphics::FORMAT::GBGR16_422_UNORM:
                return VkFormat::VK_FORMAT_G16B16G16R16_422_UNORM;

            case graphics::FORMAT::BGRG16_422_UNORM:
                return VkFormat::VK_FORMAT_B16G16R16G16_422_UNORM;

            case graphics::FORMAT::GBGR8_422_UNORM:
                return VkFormat::VK_FORMAT_G8B8G8R8_422_UNORM;

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

            case graphics::FORMAT::R64_UINT:
                return VkFormat::VK_FORMAT_R64_UINT;

            case graphics::FORMAT::R64_SINT:
                return VkFormat::VK_FORMAT_R64_SINT;

            case graphics::FORMAT::R64_SFLOAT:
                return VkFormat::VK_FORMAT_R64_SFLOAT;

            case graphics::FORMAT::RG64_UINT:
                return VkFormat::VK_FORMAT_R64G64_UINT;

            case graphics::FORMAT::RG64_SINT:
                return VkFormat::VK_FORMAT_R64G64_SINT;

            case graphics::FORMAT::RG64_SFLOAT:
                return VkFormat::VK_FORMAT_R64G64_SFLOAT;

            case graphics::FORMAT::RGB64_UINT:
                return VkFormat::VK_FORMAT_R64G64B64_UINT;

            case graphics::FORMAT::RGB64_SINT:
                return VkFormat::VK_FORMAT_R64G64B64_SINT;

            case graphics::FORMAT::RGB64_SFLOAT:
                return VkFormat::VK_FORMAT_R64G64B64_SFLOAT;

            case graphics::FORMAT::RGBA64_UINT:
                return VkFormat::VK_FORMAT_R64G64B64A64_UINT;

            case graphics::FORMAT::RGBA64_SINT:
                return VkFormat::VK_FORMAT_R64G64B64A64_SINT;

            case graphics::FORMAT::RGBA64_SFLOAT:
                return VkFormat::VK_FORMAT_R64G64B64A64_SFLOAT;

            // Depth-stenci formats.
            case graphics::FORMAT::D16_UNORM:
                return VkFormat::VK_FORMAT_D16_UNORM;

            case graphics::FORMAT::D16_UNORM_S8_UINT:
                return VkFormat::VK_FORMAT_D16_UNORM_S8_UINT;

            case graphics::FORMAT::D24_UNORM_S8_UINT:
                return VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;

            case graphics::FORMAT::X8_D24_UNORM_PACK32:
                return VkFormat::VK_FORMAT_X8_D24_UNORM_PACK32;

            case graphics::FORMAT::D32_SFLOAT:
                return VkFormat::VK_FORMAT_D32_SFLOAT;

            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
                return VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;

            // Special formats.
            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:
                return VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case graphics::FORMAT::E5BGR9_UFLOAT_PACK32:
                return VkFormat::VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

            case graphics::FORMAT::BC1_RGB_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC1_RGB_UNORM_BLOCK;

            case graphics::FORMAT::BC1_RGB_SRGB_BLOCK:
                return VkFormat::VK_FORMAT_BC1_RGB_SRGB_BLOCK;

            case graphics::FORMAT::BC1_RGBA_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

            case graphics::FORMAT::BC1_RGBA_SRGB_BLOCK:
                return VkFormat::VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

            case graphics::FORMAT::BC2_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC2_UNORM_BLOCK;

            case graphics::FORMAT::BC2_SRGB_BLOCK:
                return VkFormat::VK_FORMAT_BC2_SRGB_BLOCK;

            case graphics::FORMAT::BC3_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC3_UNORM_BLOCK;

            case graphics::FORMAT::BC3_SRGB_BLOCK:
                return VkFormat::VK_FORMAT_BC3_SRGB_BLOCK;

            case graphics::FORMAT::BC4_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC4_UNORM_BLOCK;

            case graphics::FORMAT::BC4_SNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC4_SNORM_BLOCK;

            case graphics::FORMAT::BC5_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC5_UNORM_BLOCK;

            case graphics::FORMAT::BC5_SNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC5_SNORM_BLOCK;

            case graphics::FORMAT::BC6H_UFLOAT_BLOCK:
                return VkFormat::VK_FORMAT_BC6H_UFLOAT_BLOCK;

            case graphics::FORMAT::BC6H_SFLOAT_BLOCK:
                return VkFormat::VK_FORMAT_BC6H_SFLOAT_BLOCK;

            case graphics::FORMAT::BC7_UNORM_BLOCK:
                return VkFormat::VK_FORMAT_BC7_UNORM_BLOCK;

            case graphics::FORMAT::BC7_SRGB_BLOCK:
                return VkFormat::VK_FORMAT_BC7_SRGB_BLOCK;

            case graphics::FORMAT::G8_B8_R8_3PLANE_420_UNORM:
                return VkFormat::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;

            case graphics::FORMAT::G8_B8R8_2PLANE_420_UNORM:
                return VkFormat::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;

            case graphics::FORMAT::G8_B8_R8_3PLANE_422_UNORM:
                return VkFormat::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;

            case graphics::FORMAT::G8_B8R8_2PLANE_422_UNORM:
                return VkFormat::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;

            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;

            case graphics::FORMAT::G10X6_BR10X6_2PLANE_420_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;

            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;

            case graphics::FORMAT::G10X6_BR10X6_2PLANE_422_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;

            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;

            case graphics::FORMAT::G12X4_BR12X4_2PLANE_420_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;

            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;

            case graphics::FORMAT::G12X4_BR12X4_2PLANE_422_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;

            case graphics::FORMAT::G16_B16_R16_3PLANE_420_UNORM:
                return VkFormat::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;

            case graphics::FORMAT::G16_BR16_2PLANE_420_UNORM:
                return VkFormat::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;

            case graphics::FORMAT::G16_B16_R16_3PLANE_422_UNORM:
                return VkFormat::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;

            case graphics::FORMAT::G16_BR16_2PLANE_422_UNORM:
                return VkFormat::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;

            case graphics::FORMAT::G8_B8_R8_3PLANE_444_UNORM:
                return VkFormat::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;

            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;

            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
                return VkFormat::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;

            case graphics::FORMAT::G16_B16_R16_3PLANE_444_UNORM:
                return VkFormat::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;

            case graphics::FORMAT::R10X6_UNORM_PACK16:
                return VkFormat::VK_FORMAT_R10X6_UNORM_PACK16;

            case graphics::FORMAT::RG10X6_UNORM_2PACK16:
                return VkFormat::VK_FORMAT_R10X6G10X6_UNORM_2PACK16;

            case graphics::FORMAT::RGBA10X6_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;

            case graphics::FORMAT::R12X4_UNORM_PACK16:
                return VkFormat::VK_FORMAT_R12X4_UNORM_PACK16;

            case graphics::FORMAT::RG12X4_UNORM_2PACK16:
                return VkFormat::VK_FORMAT_R12X4G12X4_UNORM_2PACK16;

            case graphics::FORMAT::RGBA12X4_UNORM_4PACK16:
                return VkFormat::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;

            default:
                return VkFormat::VK_FORMAT_MAX_ENUM;
        }
    }

    VkIndexType vulkan(graphics::INDEX_TYPE index_type) noexcept
    {
        switch (index_type) {
            case graphics::INDEX_TYPE::UNDEFINED:
                return VkIndexType::VK_INDEX_TYPE_MAX_ENUM;

            case graphics::INDEX_TYPE::UINT_16:
                return VkIndexType::VK_INDEX_TYPE_UINT16;

            case graphics::INDEX_TYPE::UINT_32:
                return VkIndexType::VK_INDEX_TYPE_UINT32;

            default:
                return VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
        }
    }

    VkFormatFeatureFlags vulkan(graphics::FORMAT_FEATURE format_feature) noexcept
    {
        VkFormatFeatureFlags result = 0;

        using E = std::underlying_type_t<graphics::FORMAT_FEATURE>;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::STORAGE_IMAGE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::STORAGE_IMAGE_ATOMIC))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::UNIFORM_TEXEL_BUFFER))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::STORAGE_TEXEL_BUFFER))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::STORAGE_TEXEL_BUFFER_ATOMIC))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::VERTEX_BUFFER))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::COLOR_ATTACHMENT))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::COLOR_ATTACHMENT_BLEND))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::DEPTH_STENCIL_ATTACHMENT))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::BLIT_SOURCE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_BLIT_SRC_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::BLIT_DESTINATION))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_BLIT_DST_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_FILTER_LINEAR))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::TRANSFER_SOURCE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::TRANSFER_DESTINATION))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::MIDPOINT_CHROMA_SAMPLES))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::DISJOINT))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DISJOINT_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::COSITED_CHROMA_SAMPLES))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT;

        if (static_cast<E>(format_feature & graphics::FORMAT_FEATURE::SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMAGE))
            result |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG;

        return result;
    }

    VkAttachmentLoadOp vulkan(graphics::ATTACHMENT_LOAD_TREATMENT load_treatment) noexcept
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

    VkAttachmentStoreOp vulkan(graphics::ATTACHMENT_STORE_TREATMENT store_treatment) noexcept
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

    VkColorSpaceKHR vulkan(graphics::COLOR_SPACE color_space) noexcept
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

    VkSampleCountFlagBits vulkan(std::uint32_t samples_count) noexcept
    {
        if (samples_count & 0x40u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_64_BIT;

        if (samples_count & 0x20u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_32_BIT;

        if (samples_count & 0x10u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_16_BIT;

        if (samples_count & 0x08u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;

        if (samples_count & 0x04u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT;

        if (samples_count & 0x02u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_2_BIT;

        if (samples_count & 0x01u)
            return VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

        return VkSampleCountFlagBits::VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    }

    VkBufferUsageFlags vulkan(graphics::BUFFER_USAGE buffer_usage) noexcept
    {
        VkBufferUsageFlags result = 0;

        using E = std::underlying_type_t<graphics::BUFFER_USAGE>;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::TRANSFER_SOURCE))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::TRANSFER_DESTINATION))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::UNIFORM_TEXEL_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::STORAGE_TEXEL_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::UNIFORM_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::STORAGE_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::INDEX_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::VERTEX_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (static_cast<E>(buffer_usage & graphics::BUFFER_USAGE::INDIRECT_BUFFER))
            result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        return result;
    }

    VkImageUsageFlags vulkan(graphics::IMAGE_USAGE image_usage) noexcept
    {
        VkImageUsageFlags result = 0;

        using E = std::underlying_type_t<graphics::IMAGE_USAGE>;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::TRANSFER_SOURCE))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::TRANSFER_DESTINATION))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::SAMPLED))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::STORAGE))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::COLOR_ATTACHMENT))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

        if (static_cast<E>(image_usage & graphics::IMAGE_USAGE::INPUT_ATTACHMENT))
            result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        return result;
    }

    VkQueueFlagBits vulkan(graphics::QUEUE_CAPABILITY queue_capability) noexcept
    {
        VkQueueFlags result = 0;

        using E = std::underlying_type_t<graphics::QUEUE_CAPABILITY>;

        if (static_cast<E>(queue_capability & graphics::QUEUE_CAPABILITY::GRAPHICS))
            result |= VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;

        if (static_cast<E>(queue_capability & graphics::QUEUE_CAPABILITY::COMPUTE))
            result |= VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;

        if (static_cast<E>(queue_capability & graphics::QUEUE_CAPABILITY::TRANSFER))
            result |= VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;

        return static_cast<VkQueueFlagBits>(result);
    }

    VkPresentModeKHR vulkan(graphics::PRESENTATION_MODE presentation_mode) noexcept
    {
        switch (presentation_mode) {
            case graphics::PRESENTATION_MODE::IMMEDIATE:
                return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;

            case graphics::PRESENTATION_MODE::MAILBOX:
                return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;

            case graphics::PRESENTATION_MODE::FIFO:
                return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;

            case graphics::PRESENTATION_MODE::FIFO_RELAXED:
                return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR;

            default:
                return VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR;
        }
    }

    VkImageAspectFlags vulkan(graphics::IMAGE_ASPECT image_aspect) noexcept
    {
        VkImageAspectFlags result = 0;

        using E = std::underlying_type_t<graphics::IMAGE_ASPECT>;

        if (static_cast<E>(image_aspect & graphics::IMAGE_ASPECT::COLOR_BIT))
            result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

        if (static_cast<E>(image_aspect & graphics::IMAGE_ASPECT::DEPTH_BIT))
            result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;

        if (static_cast<E>(image_aspect & graphics::IMAGE_ASPECT::STENCIL_BIT))
            result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;

        return result;
    }

    VkAccessFlags vulkan(graphics::MEMORY_ACCESS_TYPE access_type) noexcept
    {
        VkAccessFlags result = 0;

        using E = std::underlying_type_t<graphics::MEMORY_ACCESS_TYPE>;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::INDIRECT_COMMAND_READ))
            result |= VkAccessFlagBits::VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::INDEX_READ))
            result |= VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::VERTEX_ATTRIBUTE_READ))
            result |= VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::UNIFORM_READ))
            result |= VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::INPUT_ATTACHMENT_READ))
            result |= VkAccessFlagBits::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::SHADER_READ))
            result |= VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::SHADER_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_READ))
            result |= VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::DEPTH_STENCIL_ATTACHMENT_READ))
            result |= VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::DEPTH_STENCIL_ATTACHMENT_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::TRANSFER_READ))
            result |= VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::TRANSFER_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::HOST_READ))
            result |= VkAccessFlagBits::VK_ACCESS_HOST_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::HOST_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::MEMORY_READ))
            result |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;

        if (static_cast<E>(access_type & graphics::MEMORY_ACCESS_TYPE::MEMORY_WRITE))
            result |= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;

        return result;
    }

    VkSharingMode vulkan(graphics::RESOURCE_SHARING_MODE sharing_mode) noexcept
    {
        switch (sharing_mode) {
            case graphics::RESOURCE_SHARING_MODE::EXCLUSIVE:
                return VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

            case graphics::RESOURCE_SHARING_MODE::CONCURRENT:
                return VkSharingMode::VK_SHARING_MODE_CONCURRENT;

            default:
                return VkSharingMode::VK_SHARING_MODE_MAX_ENUM;
        }
    }
}
