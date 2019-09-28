#pragma once

#include <unordered_map>
#include <concepts>
#include <optional>
#include <vector>
#include <array>
#include <set>

#include <boost/functional/hash_fwd.hpp>

#include "device/device.hxx"

#include "graphics.hxx"
#include "vertex.hxx"
#include "pipeline_states.hxx"
#include "material.hxx"
#include "shader_program.hxx"

#define USE_DYNAMIC_PIPELINE_STATE 1


namespace graphics
{
    class vertex_input_state_manager final {
    public:

        [[nodiscard]] std::uint32_t binding_index(graphics::vertex_layout const &vertex_layout);

        [[nodiscard]] graphics::vertex_input_state const &vertex_input_state(graphics::vertex_layout const &vertex_layout);

    private:

        std::unordered_map<graphics::vertex_layout, graphics::vertex_input_state, hash<graphics::vertex_layout>> vertex_input_states_;

    };
}


namespace graphics
{
    class pipeline final {
    public:

        pipeline(VkPipeline handle) noexcept : handle_{handle} { }

        VkPipeline handle() const noexcept { return handle_; }

    private:

        VkPipeline handle_;
    };
}

namespace graphics
{
    struct pipeline_invariant final {
        std::shared_ptr<graphics::material> material_;

        // TODO:: add tesselation, viewport, multisample and dynamic states.
        graphics::pipeline_states pipeline_states_;

        // TODO:: replace by an abstraction from material.
        VkPipelineLayout layout_;

        VkRenderPass render_pass_;
        std::uint32_t subpass_index_;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, pipeline_invariant>
        auto constexpr operator== (T &&rhs) const
        {
            return material_ == rhs.material_ &&
                    pipeline_states_ == rhs.pipeline_states_ &&
                    layout_ == rhs.layout_ &&
                    render_pass_ == rhs.render_pass_ &&
                    subpass_index_ == rhs.subpass_index_;
        }
    };

    template<>
    struct graphics::hash<pipeline_invariant> {
        std::size_t operator() (pipeline_invariant const &pipeline_invariant) const
        {
            std::size_t seed = 0;

            graphics::hash<graphics::material> constexpr material_hasher;
            boost::hash_combine(seed, material_hasher(*pipeline_invariant.material_));

            graphics::hash<graphics::pipeline_states> constexpr pipeline_states_hasher;
            boost::hash_combine(seed, pipeline_states_hasher(pipeline_invariant.pipeline_states_));

            boost::hash_combine(seed, pipeline_invariant.layout_);
            boost::hash_combine(seed, pipeline_invariant.render_pass_);
            boost::hash_combine(seed, pipeline_invariant.subpass_index_);

            return seed;
        }
    };
}

namespace graphics
{
    class pipeline_factory final {
    public:

        pipeline_factory(VulkanDevice &vulkan_device, graphics::shader_manager &shader_manager) noexcept
            : vulkan_device_{vulkan_device}, shader_manager_{shader_manager} { }

        [[nodiscard]] std::shared_ptr<graphics::pipeline>
        create_pipeline(std::shared_ptr<graphics::material> material, graphics::pipeline_states const &pipeline_states,
                        VkPipelineLayout layout, VkRenderPass render_pass, std::uint32_t subpass_index);

    private:

        VulkanDevice &vulkan_device_;
        graphics::shader_manager &shader_manager_;

        std::unordered_map<graphics::pipeline_invariant, std::shared_ptr<graphics::pipeline>, graphics::hash<pipeline_invariant>> pipelines_;
        std::unordered_map<std::shared_ptr<graphics::pipeline>, graphics::pipeline_invariant> invariants_;
    };
}
