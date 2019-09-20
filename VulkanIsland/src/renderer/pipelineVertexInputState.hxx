#pragma once

#include <optional>

#include "main.hxx"
#include "utility/helpers.hxx"
#include "device/device.hxx"
#include "staging.hxx"
#include "renderer/graphics.hxx"
#include "renderer/vertex.hxx"


struct PipelineVertexInputState final {
    std::uint32_t binding;

    VkPipelineVertexInputStateCreateInfo info;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};


class PipelineVertexInputStatesManager final {
public:

    [[nodiscard]] std::uint32_t binding(graphics::vertex_layout const &layout) noexcept;
    [[nodiscard]] VkPipelineVertexInputStateCreateInfo const &info(graphics::vertex_layout const &layout) noexcept;

private:

    std::unordered_map<graphics::vertex_layout, PipelineVertexInputState, graphics::hash<graphics::vertex_layout>> layouts_;

    void createPipelineVertexInputState(graphics::vertex_layout const &layout) noexcept;
};
