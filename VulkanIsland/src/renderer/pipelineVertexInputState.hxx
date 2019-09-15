#pragma once

#include <optional>

#include "main.hxx"
#include "utility/helpers.hxx"
#include "device/device.hxx"
#include "staging.hxx"


struct PipelineVertexInputState final {
    std::uint32_t binding;

    VkPipelineVertexInputStateCreateInfo info;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};


class PipelineVertexInputStatesManager final {
public:

    [[nodiscard]] std::uint32_t binding(xformat::vertex_layout const &layout) noexcept;
    [[nodiscard]] VkPipelineVertexInputStateCreateInfo const &info(xformat::vertex_layout const &layout) noexcept;

private:

    std::unordered_map<xformat::vertex_layout, PipelineVertexInputState, xformat::hash_value, xformat::equal_comparator> layouts_;

    void createPipelineVertexInputState(xformat::vertex_layout const &layout) noexcept;
};
