#include "graphics_api.hxx"


namespace graphics_api
{
    VkPrimitiveTopology constexpr vulkan::operator() (graphics::PRIMITIVE_TOPOLOGY topology) const noexcept
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

    VkCullModeFlags constexpr vulkan::operator() (graphics::CULL_MODE cull_mode) const noexcept
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

    VkPolygonMode constexpr vulkan::operator() (graphics::POLYGON_MODE polygon_mode) const noexcept
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

    VkFrontFace constexpr vulkan::operator() (graphics::POLYGON_FRONT_FACE front_face) const noexcept
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

    VkCompareOp constexpr vulkan::operator() (graphics::COMPARE_OPERATION compare_operation) const noexcept
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

    VkBlendFactor constexpr vulkan::operator() (graphics::BLEND_FACTOR blend_factor) const noexcept
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

    VkBlendOp constexpr vulkan::operator() (graphics::BLEND_OPERATION blend_operation) const noexcept
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

    VkColorComponentFlags constexpr vulkan::operator() (graphics::COLOR_COMPONENT color_component) const noexcept
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

    VkShaderStageFlagBits constexpr vulkan::operator() (graphics::PIPELINE_SHADER_STAGE shader_stage) const noexcept
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
}