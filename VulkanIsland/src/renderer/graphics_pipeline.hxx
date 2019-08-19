#pragma once

#include <optional>
#include <functional>
#include <array>
#include <set>

#include <boost/functional/hash_fwd.hpp>

#include "device/device.hxx"

#include "graphics.hxx"
#include "vertex.hxx"
#include "pipeline_states.hxx"
#include "shader_program.hxx"


namespace graphics
{
class pipeline final {
public:

    VkPipeline handle() const noexcept { return handle_; }

    template<class T, typename std::enable_if_t<std::is_same_v<pipeline, std::decay_t<T>>>* = nullptr>
    auto constexpr operator== (T &&rhs) const
    {
        return primitive_topology_ == rhs.topology_ &&
            vertex_input_state_ == rhs.vertex_input_state_ &&
            rasterization_state_ == rhs.rasterization_state &&
            depth_stencil_state_ == rhs.depth_stencil_state &&
            color_blend_state_ == rhs.color_blend_state;
    }

private:

    VkPipeline handle_;

    std::vector<graphics::shader_stage> shader_stages_;

    graphics::PRIMITIVE_TOPOLOGY primitive_topology_;

    graphics::vertex_input_state vertex_input_state_;

    graphics::rasterization_state rasterization_state_;
    graphics::depth_stencil_state depth_stencil_state_;

    graphics::color_blend_state color_blend_state_;
    std::vector<graphics::color_blend_attachment_state> color_blend_attachments_;

    // TODO:: add tesselation, viewport, multisample and dynamic states.

    // TODO:: replace by an abstraction.
    VkPipelineLayout layout_;

    VkRenderPass render_pass_;
    std::uint32_t subpass_index;

    friend graphics::hash<pipeline>;
};
}

namespace graphics
{
    class pipeline_manager final {
    public:

        pipeline_manager(VulkanDevice &vulkan_device) noexcept : vulkan_device_{vulkan_device} { }

        [[nodiscard]] std::shared_ptr<graphics::pipeline> create_pipeline(std::shared_ptr<class graphics::material> material);

    private:

        VulkanDevice &vulkan_device_;

        // GAPI 
        std::map<graphics::rasterization_state, VkPipelineColorBlendStateCreateInfo> rasterization_states_;
        std::map<graphics::depth_stencil_state, VkPipelineDepthStencilStateCreateInfo> depth_stencil_states_;

        std::map<graphics::color_blend_state, VkPipelineColorBlendStateCreateInfo> color_blend_states_;
        std::map<graphics::color_blend_attachment_state, VkPipelineColorBlendAttachmentState> color_blend_attachment_states_;
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::pipeline> {
        std::size_t operator() (graphics::pipeline const &pipeline) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, pipeline.primitive_topology_);

            graphics::hash<graphics::vertex_input_state> constexpr vertex_input_state_hasher;
            boost::hash_combine(seed, vertex_input_state_hasher(pipeline.vertex_input_state_));

            graphics::hash<graphics::rasterization_state> constexpr rasterization_state_hasher;
            boost::hash_combine(seed, rasterization_state_hasher(pipeline.rasterization_state_));

            graphics::hash<graphics::depth_stencil_state> constexpr depth_stencil_state_hasher;
            boost::hash_combine(seed, depth_stencil_state_hasher(pipeline.depth_stencil_state_));

            graphics::hash<graphics::color_blend_state> constexpr color_blend_state_hasher;
            boost::hash_combine(seed, color_blend_state_hasher(pipeline.color_blend_state_));

            boost::hash_combine(seed, pipeline.layout_);
            boost::hash_combine(seed, pipeline.render_pass_);
            boost::hash_combine(seed, pipeline.subpass_index);

            return seed;
        }
    };
}
