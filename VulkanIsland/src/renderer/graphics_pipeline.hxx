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

        VkPipeline handle() const noexcept { return handle_; }

        template<class T> requires std::same_as<std::remove_cvref_t<T>, pipeline>
            auto constexpr operator== (T &&rhs) const
            {
                return handle_ == pipeline.handle_ &&
                       pipeline_states_ == rhs.pipeline_states_ &&
                       layout_ == rhs.layout_ &&
                       render_pass_ == rhs.render_pass_ &&
                       subpass_index_ == rhs.subpass_index_;
            }

    private:

        VkPipeline handle_;

        std::shared_ptr<graphics::material> material_;

        // TODO:: add tesselation, viewport, multisample and dynamic states.
        graphics::pipeline_states pipeline_states_;

        // TODO:: replace by an abstraction from material.
        VkPipelineLayout layout_;

        VkRenderPass render_pass_;
        std::uint32_t subpass_index_;

        friend graphics::hash<graphics::pipeline>;
    };

    template<>
    struct hash<graphics::pipeline> {
        std::size_t operator() (graphics::pipeline const &pipeline) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, pipeline.handle_);

            graphics::hash<graphics::material> constexpr material_hasher;
            boost::hash_combine(seed, material_hasher(*pipeline.material_));

            graphics::hash<graphics::pipeline_states> constexpr pipeline_states_hasher;
            boost::hash_combine(seed, pipeline_states_hasher(pipeline.pipeline_states_));

            boost::hash_combine(seed, pipeline.layout_);
            boost::hash_combine(seed, pipeline.render_pass_);
            boost::hash_combine(seed, pipeline.subpass_index_);

            return seed;
        }
    };
}
namespace graphics
{
    class pipeline_factory final {
    public:

        pipeline_factory(VulkanDevice &vulkan_device) noexcept : vulkan_device_{vulkan_device} { }

        [[nodiscard]] std::shared_ptr<graphics::pipeline>
        create_pipeline(std::shared_ptr<graphics::material> material, graphics::pipeline_states const &pipeline_states,
                        VkPipelineLayout layout, VkRenderPass render_pass, std::uint32_t subpass_index);

    private:

        VulkanDevice &vulkan_device_;

        struct pipeline_key final {

            pipeline_key(graphics::material const &material, graphics::pipeline_states const &pipeline_states,
                         VkPipelineLayout layout, VkRenderPass render_pass, std::uint32_t subpass_index) noexcept
                : material_{material}, pipeline_states_{pipeline_states},
                  layout_{layout}, render_pass_{render_pass}, subpass_index_{subpass_index} { };

            graphics::material const &material_;

            graphics::pipeline_states const &pipeline_states_;

            VkPipelineLayout layout_;

            VkRenderPass render_pass_;
            std::uint32_t subpass_index_;

            std::size_t operator() () const
            {
                std::size_t seed = 0;

                graphics::hash<graphics::material> constexpr material_hasher;
                boost::hash_combine(seed, material_hasher(material_));

                graphics::hash<graphics::pipeline_states> constexpr pipeline_states_hasher;
                boost::hash_combine(seed, pipeline_states_hasher(pipeline_states_));

                boost::hash_combine(seed, layout_);
                boost::hash_combine(seed, render_pass_);
                boost::hash_combine(seed, subpass_index_);

                return seed;
            }
        };

        std::unordered_map<pipeline_key, std::shared_ptr<graphics::pipeline>> pipelines_;
    };
}
