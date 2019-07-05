#include "pipelineVertexInputState.hxx"
#include "vertexFormat.hxx"


std::uint32_t PipelineVertexInputStatesManager::binding(xformat::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout) == 0)
        createPipelineVertexInputState(layout);

    return layouts_[layout].binding;
}

VkPipelineVertexInputStateCreateInfo const &PipelineVertexInputStatesManager::info(xformat::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout) == 0)
        createPipelineVertexInputState(layout);

    return layouts_[layout].info;
}

void PipelineVertexInputStatesManager::createPipelineVertexInputState(xformat::vertex_layout const &layout) noexcept
{
    auto const binding = static_cast<std::uint32_t>(std::size(layouts_));

    PipelineVertexInputState inputState;

    inputState.bindingDescriptions.push_back(
        VkVertexInputBindingDescription{binding, static_cast<std::uint32_t>(layout.sizeInBytes), VK_VERTEX_INPUT_RATE_VERTEX}
    );

    std::transform(std::cbegin(layout.attributes), std::cend(layout.attributes),
                   std::back_inserter(inputState.attributeDescriptions), [binding] (auto &&attribute)
    {
        auto format = std::visit([normalized = attribute.normalized] (auto &&type)
        {
            using T = std::decay_t<decltype(type)>;
            return getFormat<T::number, typename T::type>(normalized);

        }, attribute.type);

        auto location = std::visit([] (auto semantic)
        {
            using S = std::decay_t<decltype(semantic)>;
            return S::index;

        }, attribute.semantic);

        return VkVertexInputAttributeDescription{
            location, binding, format, static_cast<std::uint32_t>(attribute.offsetInBytes)
        };
    });

    inputState.bindingDescriptions.shrink_to_fit();
    inputState.attributeDescriptions.shrink_to_fit();
    
    inputState.binding = binding;

    inputState.info = VkPipelineVertexInputStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(inputState.bindingDescriptions)), std::data(inputState.bindingDescriptions),
        static_cast<std::uint32_t>(std::size(inputState.attributeDescriptions)), std::data(inputState.attributeDescriptions),
    };

    layouts_.emplace(layout, std::move(inputState));
}
