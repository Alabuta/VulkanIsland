#pragma once

#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>

#include "main.hxx"
#include "helpers.hxx"
#include "mesh.hxx"


class VertexInputStateInfo final {
public:

    VertexInputStateInfo(vertex_layout_t const &layout) noexcept
    {
        auto vertexSize = std::accumulate(std::cbegin(layout), std::cend(layout), 0u,
                                          [] (std::uint32_t size, auto &&description)
        {
            return size + std::visit([] (auto &&attribute)
            {
                using T = std::decay_t<decltype(attribute)>;
                return static_cast<std::uint32_t>(sizeof(T));

            }, description.attribute);
        });

        std::uint32_t binding = 0;

        inputBindingDescriptions.push_back(
            VkVertexInputBindingDescription{ binding, vertexSize, VK_VERTEX_INPUT_RATE_VERTEX }
        );

        std::transform(std::cbegin(layout), std::cend(layout),
                       std::back_inserter(attributeDescriptions), [binding] (auto &&description)
        {
            auto format = std::visit([normalized = description.normalized] (auto &&attribute)
            {
                using T = std::decay_t<decltype(attribute)>;
                return getFormat<T::number, T::type>(normalized);

            }, description.attribute);

            auto location = std::visit([] (auto semantic)
            {
                using S = std::decay_t<decltype(semantic)>;
                return S::index;

            }, description.semantic);

            return VkVertexInputAttributeDescription{
                location, binding, format, static_cast<std::uint32_t>(description.offset)
            };
        });

        info_ = VkPipelineVertexInputStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(inputBindingDescriptions)), std::data(inputBindingDescriptions),
            static_cast<std::uint32_t>(std::size(attributeDescriptions)), std::data(attributeDescriptions),
        };
    }

    VkPipelineVertexInputStateCreateInfo const &info() const noexcept { return info_; }

private:

    VkPipelineVertexInputStateCreateInfo info_;

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
