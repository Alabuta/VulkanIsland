#pragma once

#include <unordered_map>
#include <optional>
#include <iostream>
#include <vector>
#include <array>
#include <span>
#include <set>

#include "utility/mpl.hxx"
#include "utility/exceptions.hxx"
#include "vulkan/device.hxx"
#include "renderer/config.hxx"
#include "renderer/material.hxx"

#include "graphics.hxx"
#include "vertex.hxx"
#include "pipeline_states.hxx"
#include "shader_program.hxx"
#include "render_pass.hxx"
#include "descriptors.hxx"


#define USE_DYNAMIC_PIPELINE_STATE 1


namespace graphics
{
    class vertex_input_state_manager final {
    public:

        [[nodiscard]] std::uint32_t binding_index(graphics::vertex_layout const &vertex_layout);

        [[nodiscard]] graphics::vertex_input_state const &vertex_input_state(graphics::vertex_layout const &vertex_layout);

        [[nodiscard]] graphics::vertex_input_state
        get_adjusted_vertex_input_state(graphics::vertex_layout const &vertex_layout, graphics::vertex_layout const &required_vertex_layout) const;

    private:

        std::unordered_map<graphics::vertex_layout, graphics::vertex_input_state, hash<graphics::vertex_layout>> vertex_input_states_;

    };
}

namespace graphics
{
    class pipeline_layout final {
    public:

        pipeline_layout(VkPipelineLayout handle) noexcept : handle_{handle} { }

        VkPipelineLayout handle() const noexcept { return handle_; }

    private:

        VkPipelineLayout handle_;
    };

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

        std::shared_ptr<graphics::render_pass> render_pass_;
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
    struct hash<pipeline_invariant> {
        std::size_t operator() (pipeline_invariant const &pipeline_invariant) const;
    };
}

namespace graphics
{
    class pipeline_factory final {
    public:

        pipeline_factory(vulkan::device &device, renderer::config const &renderer_config, graphics::shader_manager &shader_manager) noexcept
            : device_{device}, renderer_config_{renderer_config}, shader_manager_{shader_manager} { }

        [[nodiscard]] std::shared_ptr<graphics::pipeline>
        create_pipeline(std::shared_ptr<graphics::material> material, graphics::pipeline_states const &pipeline_states,
                        VkPipelineLayout layout, std::shared_ptr<graphics::render_pass> render_pass, std::uint32_t subpass_index);

        [[nodiscard]] std::shared_ptr<graphics::pipeline_layout>
        create_pipeline_layout(std::span<const graphics::descriptor_set_layout> const descriptor_set_layouts);

    private:

        vulkan::device &device_;
        renderer::config renderer_config_;
        graphics::shader_manager &shader_manager_;

        std::unordered_map<graphics::pipeline_invariant, std::shared_ptr<graphics::pipeline>, graphics::hash<pipeline_invariant>> pipelines_;
    };
}

/* [[nodiscard]] */ std::optional<VkPipelineLayout>
create_pipeline_layout(vulkan::device const &device, std::span<VkDescriptorSetLayout const> const descriptorSetLayouts);
