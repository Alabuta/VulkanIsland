#pragma once

#include <optional>

#include "main.hxx"
#include "helpers.hxx"
#include "device.hxx"
#include "staging.hxx"


struct VertexInputStateInfo final {

    std::uint32_t binding;

    VkPipelineVertexInputStateCreateInfo info;

    std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};


class VertexLayoutsManager final {
public:

    [[nodiscard]] std::uint32_t binding(xformat::vertex_layout const &layout) noexcept;
    [[nodiscard]] VkPipelineVertexInputStateCreateInfo const &info(xformat::vertex_layout const &layout) noexcept;

private:

    std::unordered_map<xformat::vertex_layout, VertexInputStateInfo, xformat::hash_value, xformat::equal_comparator> layouts_;


};
