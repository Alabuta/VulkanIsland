#pragma once

#include <optional>
#include <functional>
#include <array>

#include <boost/functional/hash_fwd.hpp>

#include "main.hxx"
#include "helpers.hxx"
#include "device/device.hxx"
#include "graphics.hxx"
#include "vertex.hxx"
#include "pipeline_states.hxx"
#include "staging.hxx"
#include "material.hxx"
#include "pipelineVertexInputState.hxx"


#define USE_DYNAMIC_PIPELINE_STATE 1


class GraphicsPipeline final {
public:

    GraphicsPipeline(VkPipeline handle) noexcept : handle_{handle} { }

    VkPipeline handle() const noexcept { return handle_; }

private:

    VkPipeline handle_;
};


class GraphicsPipelineManager final {
public:

    GraphicsPipelineManager(VulkanDevice &vulkanDevice, MaterialFactory &materialFactory, PipelineVertexInputStatesManager &pipelineVertexInputStatesManager) noexcept :
        vulkanDevice_{vulkanDevice}, materialFactory_{materialFactory}, pipelineVertexInputStatesManager_{pipelineVertexInputStatesManager} { }

    [[nodiscard]] std::shared_ptr<GraphicsPipeline>
    CreateGraphicsPipeline(xformat::vertex_layout const &layout, std::shared_ptr<Material> material, PRIMITIVE_TOPOLOGY topology,
                           VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkExtent2D extent);

    auto const &graphicsPipelines() const noexcept { return graphicsPipelineProperties_; }

    struct GraphicsPipelinePropertiesKey final {
        PRIMITIVE_TOPOLOGY topology_;
        xformat::vertex_layout layout_;
        std::shared_ptr<Material> material_;

        std::array<float, 2> viewport_;

        struct hash_value final {
            template<class T>
            std::size_t constexpr operator() (T &&graphicsPipeline) const noexcept
            {
                auto seed = xformat::hash_value{}(graphicsPipeline.layout_);

                boost::hash_combine(seed, graphicsPipeline.topology_);

                boost::hash_combine(seed, graphicsPipeline.material_);

            #if !USE_DYNAMIC_PIPELINE_STATE
                boost::hash_combine(seed, std::hash<float>{}(graphicsPipeline.viewport_[0]));
                boost::hash_combine(seed, std::hash<float>{}(graphicsPipeline.viewport_[1]));
            #endif

                return seed;
            }
        };

        struct equal_comparator final {
            template<class T1, class T2>
            std::size_t constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
            {
                auto topology = lhs.topology_ == rhs.topology_;

                auto layout = xformat::equal_comparator{}(lhs.layout_, rhs.layout_);

                auto material = lhs.material_ == rhs.material_;

            #if USE_DYNAMIC_PIPELINE_STATE
                return topology && layout && material;
            #else
                auto viewport = lhs.viewport_ == rhs.viewport_;

                return topology && layout && material && viewport;
            #endif
            }
        };

    #if TEMPORARILY_DISABLED
        struct less_comparator final {
            template<class T1, class T2, typename std::enable_if_t<are_same_v<GraphicsPipelinePropertiesKey, T1, T2>>* = nullptr>
            auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
            {
                auto topology = lhs.topology_ < rhs.topology_;

                auto layout = xformat::less_comparator{}(lhs.layout_, rhs.layout_);

                auto material = lhs.material_ < rhs.material_;

                return topology && layout && material;
            }
        };
    #endif
    };

private:

    VulkanDevice &vulkanDevice_;
    MaterialFactory &materialFactory_;
    PipelineVertexInputStatesManager &pipelineVertexInputStatesManager_;

    std::unordered_map<GraphicsPipelinePropertiesKey, std::shared_ptr<GraphicsPipeline>,
        GraphicsPipelinePropertiesKey::hash_value, GraphicsPipelinePropertiesKey::equal_comparator> graphicsPipelineProperties_;
};


template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>* = nullptr>
[[nodiscard]] std::optional<VkPipelineLayout>
CreatePipelineLayout(VulkanDevice const &vulkanDevice, T &&descriptorSetLayouts) noexcept
{
    static_assert(
        std::is_same_v<typename std::decay_t<T>::value_type, VkDescriptorSetLayout>,
        "container has to contain VkDescriptorSetLayout elements"
    );

    VkPipelineLayoutCreateInfo const layoutCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(descriptorSetLayouts)), std::data(descriptorSetLayouts),
        0, nullptr
    };

    std::optional<VkPipelineLayout> pipelineLayout;

    VkPipelineLayout handle;

    if (auto result = vkCreatePipelineLayout(vulkanDevice.handle(), &layoutCreateInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create pipeline layout: "s << result << '\n';

    else pipelineLayout.emplace(handle);

    return pipelineLayout;
}


namespace graphics
{
class pipeline final {
public:

    VkPipeline handle() const noexcept { return handle_; }

    template<class T, typename std::enable_if_t<std::is_same_v<pipeline, std::decay_t<T>>>* = nullptr>
    auto constexpr operator== (T && rhs) const
    {
        return topology_ == rhs.topology_ &&
            vertex_layout_ == rhs.vertex_layout_;
    }

private:

    VkPipeline handle_;

    PRIMITIVE_TOPOLOGY topology_;
    graphics::vertex_layout vertex_layout_;

    graphics::rasterization_state rasterization_state;
    graphics::depth_stencil_state depth_stencil_state;
    graphics::color_blend_state color_blend_state;

    friend graphics::hash<pipeline>;
};
}

namespace graphics
{
class pipeline_manager final {
public:

    pipeline_manager(VulkanDevice &vulkan_device) noexcept : vulkan_device_{vulkan_device} { }

private:

    VulkanDevice &vulkan_device_;
};
}

namespace graphics
{
    template<>
    struct hash<graphics::pipeline> {
        std::size_t operator() (graphics::pipeline const &pipeline) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, pipeline.topology_);

            graphics::hash<graphics::vertex_layout> constexpr vertex_layout_hasher;
            boost::hash_combine(seed, vertex_layout_hasher(pipeline.vertex_layout_));

            graphics::hash<graphics::rasterization_state> constexpr rasterization_state_hasher;
            boost::hash_combine(seed, rasterization_state_hasher(pipeline.rasterization_state));

            graphics::hash<graphics::depth_stencil_state> constexpr depth_stencil_state_hasher;
            boost::hash_combine(seed, depth_stencil_state_hasher(pipeline.depth_stencil_state));

            graphics::hash<graphics::color_blend_state> constexpr color_blend_state_hasher;
            boost::hash_combine(seed, color_blend_state_hasher(pipeline.color_blend_state));

            return seed;
        }
    };
}
