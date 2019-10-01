#include "renderer/graphics_api.hxx"
#include "pipelineVertexInputState.hxx"


std::uint32_t PipelineVertexInputStatesManager::binding_index(graphics::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout) == 0)
        createPipelineVertexInputState(layout);

    return layouts_[layout].binding_index;
}

VkPipelineVertexInputStateCreateInfo const &PipelineVertexInputStatesManager::info(graphics::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout) == 0)
        createPipelineVertexInputState(layout);

    return layouts_[layout].info;
}

void PipelineVertexInputStatesManager::createPipelineVertexInputState(graphics::vertex_layout const &layout) noexcept
{
    auto const binding_index = static_cast<std::uint32_t>(std::size(layouts_));

    PipelineVertexInputState inputState;

    inputState.bindingDescriptions.push_back(
        VkVertexInputBindingDescription{binding_index, static_cast<std::uint32_t>(layout.size_in_bytes), VK_VERTEX_INPUT_RATE_VERTEX}
    );

    std::transform(std::cbegin(layout.attributes), std::cend(layout.attributes),
                   std::back_inserter(inputState.attributeDescriptions), [binding_index] (auto &&attribute)
    {
        auto format = graphics::get_vertex_attribute_format(attribute);

        auto location = graphics::get_vertex_attribute_semantic_index(attribute);

        return VkVertexInputAttributeDescription{
            location, binding_index, convert_to::vulkan(format), static_cast<std::uint32_t>(attribute.offset_in_bytes)
        };
    });

    inputState.bindingDescriptions.shrink_to_fit();
    inputState.attributeDescriptions.shrink_to_fit();
    
    inputState.binding_index = binding_index;

    inputState.info = VkPipelineVertexInputStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(inputState.bindingDescriptions)), std::data(inputState.bindingDescriptions),
        static_cast<std::uint32_t>(std::size(inputState.attributeDescriptions)), std::data(inputState.attributeDescriptions),
    };

    layouts_.emplace(layout, std::move(inputState));
}
