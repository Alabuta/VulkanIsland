#pragma once

#include <optional>

#include "main.hxx"
#include "helpers.hxx"
#include "device.hxx"
#include "staging.hxx"

class VertexInputStateInfo final {
public:

    VertexInputStateInfo(vertex_layout_t const &layout) noexcept;

    VkPipelineVertexInputStateCreateInfo const &info() const noexcept { return info_; }

    template<class T>
    bool operator== (T &&rhs) const noexcept;

    template<class T>
    std::size_t hash_value() const noexcept;

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
