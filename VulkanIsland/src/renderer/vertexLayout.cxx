#include "vertexLayout.hxx"
#include "vertexFormat.hxx"


namespace
{
VertexInputStateInfo CreateVertexInput(xformat::vertex_layout const &layout, std::uint32_t binding) noexcept
{
    VertexInputStateInfo inputState;

    inputState.inputBindingDescriptions.push_back(
        VkVertexInputBindingDescription{binding, static_cast<std::uint32_t>(layout.sizeInBytes), VK_VERTEX_INPUT_RATE_VERTEX}
    );

    std::transform(std::cbegin(layout.attributes), std::cend(layout.attributes),
                   std::back_inserter(inputState.attributeDescriptions), [binding] (auto &&attribute)
    {
        auto format = std::visit([normalized = attribute.normalized](auto &&type)
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

    inputState.info = VkPipelineVertexInputStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(inputState.inputBindingDescriptions)), std::data(inputState.inputBindingDescriptions),
        static_cast<std::uint32_t>(std::size(inputState.attributeDescriptions)), std::data(inputState.attributeDescriptions),
    };

    inputState.inputBindingDescriptions.shrink_to_fit();
    inputState.attributeDescriptions.shrink_to_fit();

    return inputState;
}
}


std::uint32_t VertexLayoutsManager::binding(xformat::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout))
        return layouts_.at(layout).binding;

    auto const binding = static_cast<std::uint32_t>(std::size(layouts_));

    auto inputState = CreateVertexInput(layout, binding);

    layouts_[layout] = std::move(inputState);

    return binding;
}

VkPipelineVertexInputStateCreateInfo const &VertexLayoutsManager::info(xformat::vertex_layout const &layout) noexcept
{
    if (layouts_.count(layout))
        return layouts_.at(layout).info;

    auto const binding = static_cast<std::uint32_t>(std::size(layouts_));

    auto inputState = CreateVertexInput(layout, binding);

    layouts_[layout] = std::move(inputState);

    return layouts_[layout].info;
}
