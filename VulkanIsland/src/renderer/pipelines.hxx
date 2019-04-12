#pragma once

#include <optional>

#include "main.hxx"
#include "helpers.hxx"
#include "device.hxx"
#include "staging.hxx"
#include "material.hxx"
#include "pipelineVertexInputState.hxx"



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

private:

    VulkanDevice &vulkanDevice_;
    MaterialFactory &materialFactory_;
    PipelineVertexInputStatesManager &pipelineVertexInputStatesManager_;

    struct GraphicsPipelinePropertiesKey final {
        PRIMITIVE_TOPOLOGY topology_;
        xformat::vertex_layout layout_;
        std::shared_ptr<Material> material_;

        struct hash_value final {
            template<class T, typename std::enable_if_t<std::is_same_v<GraphicsPipelinePropertiesKey, std::decay_t<T>>>...>
            auto constexpr operator() (T &&graphicsPipeline) const noexcept
            {
                std::size_t seed = 0;

                boost::hash_combine(seed, graphicsPipeline.topology_);

                auto layout = xformat::hash_value{}(graphicsPipeline.layout_);

                boost::hash_combine(seed, graphicsPipeline.material_);

                return seed;
            }
        };

        struct equal_comparator final {
            template<class T1, class T2, typename std::enable_if_t<are_same_v<GraphicsPipelinePropertiesKey, T1, T2>>...>
            auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
            {
                auto topology = lhs.topology_ == rhs.topology_;

                auto layout = xformat::equal_comparator{}(lhs.layout_, rhs.layout_);

                auto material = lhs.material_ == rhs.material_;

                return topology && layout && material;
            }
        };

        struct less_comparator final {
            template<class T1, class T2, typename std::enable_if_t<are_same_v<GraphicsPipelinePropertiesKey, T1, T2>>...>
            auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
            {
                auto topology = lhs.topology_ < rhs.topology_;

                auto layout = xformat::less_comparator{}(lhs.layout_, rhs.layout_);

                auto material = lhs.material_ < rhs.material_;

                return topology && layout && material;
            }
        };
    };

    /*struct GraphicsPipelinePropertiesValue final {
        std::shared_ptr<MaterialProperties> materialProperties_;
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStages_;
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputInfo_;
        VkPipelineInputAssemblyStateCreateInfo vertexAssemblyStateCreateInfo_;
    };*/

    std::unordered_map<GraphicsPipelinePropertiesKey, std::shared_ptr<GraphicsPipeline>,
        GraphicsPipelinePropertiesKey::hash_value, GraphicsPipelinePropertiesKey::equal_comparator> graphicsPipelineProperties_;
};


class VertexInputStateInfo2 final {
public:

    VertexInputStateInfo2(vertex_layout_t const &layout, std::uint32_t binding = 5) noexcept;

    VkPipelineVertexInputStateCreateInfo const &info() const noexcept { return info_; }

    std::uint32_t binding() const noexcept { return binding_; }

    /*template<class T>
    bool operator== (T &&rhs) const noexcept;

    template<class T>
    std::size_t hash_value() const noexcept;*/

private:

    VkPipelineVertexInputStateCreateInfo info_;
    std::uint32_t binding_;

    std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};


template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
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
