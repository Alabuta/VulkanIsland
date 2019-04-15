#include <vector>
#include <iostream>
#include <algorithm>

#include <boost/functional/hash_fwd.hpp>

#include "pipelines.hxx"


namespace
{
VkPrimitiveTopology constexpr ConvertToGAPI(PRIMITIVE_TOPOLOGY topology) noexcept
{
    switch (topology) {
        case PRIMITIVE_TOPOLOGY::POINTS:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        case PRIMITIVE_TOPOLOGY::LINES:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        case PRIMITIVE_TOPOLOGY::LINE_STRIP:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

        case PRIMITIVE_TOPOLOGY::TRIANGLES:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        case PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        case PRIMITIVE_TOPOLOGY::TRIANGLE_FAN:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

        default:
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}
}


std::shared_ptr<GraphicsPipeline>
GraphicsPipelineManager::CreateGraphicsPipeline(xformat::vertex_layout const &layout, std::shared_ptr<Material> material, PRIMITIVE_TOPOLOGY topology,
                                                VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkExtent2D extent)
{
    auto viewportExtent = std::array{static_cast<float>(extent.width), static_cast<float>(extent.height)};

    GraphicsPipelinePropertiesKey key{topology, layout, material, viewportExtent };

    if (graphicsPipelineProperties_.count(key) == 0) {
        auto materialProperties = materialFactory_.properties(material);

        if (!materialProperties)
            throw std::runtime_error("failed to get a material properties"s);

        auto &&shaderStages = materialFactory_.pipelineShaderStages(material);

        if (std::empty(shaderStages))
            throw std::runtime_error("material's shader stages are empty"s);

        // Vertex layout
        auto &&pipelineVertexInputInfo = pipelineVertexInputStatesManager_.info(layout);

        // TODO:: primitive topology
        VkPipelineInputAssemblyStateCreateInfo const vertexAssemblyStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            nullptr, 0,
            ConvertToGAPI(topology),
            VK_FALSE
        };

        // Render pass
    #if USE_DYNAMIC_PIPELINE_STATE
        auto const dynamicStates = std::array{
            VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
            VkDynamicState::VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo const dynamicStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(dynamicStates)),
            std::data(dynamicStates)
        };

        auto constexpr rasterizerDiscardEnable = VK_TRUE;
    #else
        VkViewport const viewport{
            0, static_cast<float>(extent.height),
            static_cast<float>(extent.width), -static_cast<float>(extent.height),
            0, 1
        };

        VkRect2D const scissor{
            {0, 0}, extent
        };

        VkPipelineViewportStateCreateInfo const viewportStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            nullptr, 0,
            1, &viewport,
            1, &scissor
        };
    #endif

        VkPipelineMultisampleStateCreateInfo const multisampleCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr, 0,
            vulkanDevice_.samplesCount(),//VK_SAMPLE_COUNT_1_BIT
            VK_FALSE, 1,
            nullptr,
            VK_FALSE,
            VK_FALSE
        };

        VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            nullptr,
            VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
            static_cast<std::uint32_t>(std::size(shaderStages)), std::data(shaderStages),
            &pipelineVertexInputInfo,
            &vertexAssemblyStateCreateInfo,
            nullptr,
    #if USE_DYNAMIC_PIPELINE_STATE
            nullptr,
    #else
            &viewportStateCreateInfo,
    #endif
            &materialProperties->rasterizationState,
            &multisampleCreateInfo,
            &materialProperties->depthStencilState,
            &materialProperties->colorBlendState,
    #if USE_DYNAMIC_PIPELINE_STATE
            &dynamicStateCreateInfo,
    #else
            nullptr,
    #endif
            pipelineLayout,
            renderPass,
            0,
            VK_NULL_HANDLE, -1
        };

        VkPipeline handle;

        if (auto result = vkCreateGraphicsPipelines(vulkanDevice_.handle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline: "s + std::to_string(result));

        auto graphicsPipeline = std::shared_ptr<GraphicsPipeline>(
            new GraphicsPipeline{handle}, [this] (GraphicsPipeline *const ptr_pipeline)
            {
                vkDestroyPipeline(vulkanDevice_.handle(), ptr_pipeline->handle(), nullptr);

                delete ptr_pipeline;
            }
        );

        graphicsPipelineProperties_.emplace(key, graphicsPipeline);
    }

    return graphicsPipelineProperties_.at(key);
}

#if NOT_YET_IMPLEMENTED
namespace
{
    bool operator== (VkVertexInputBindingDescription const &lhs, VkVertexInputBindingDescription const &rhs) noexcept
    {
        return lhs.binding == rhs.binding && lhs.stride == rhs.stride && lhs.inputRate == rhs.inputRate;
    }

    bool operator== (VkVertexInputAttributeDescription const &lhs, VkVertexInputAttributeDescription const &rhs) noexcept
    {
        return lhs.location == rhs.location && lhs.binding == rhs.binding && lhs.format == rhs.format && lhs.offset == rhs.offset;
    }
}
#endif


VertexInputStateInfo2::VertexInputStateInfo2(vertex_layout_t const &layout, std::uint32_t binding) noexcept : binding_{binding}
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

    inputBindingDescriptions.push_back(
        VkVertexInputBindingDescription{ binding, vertexSize, VK_VERTEX_INPUT_RATE_VERTEX }
    );

    std::transform(std::cbegin(layout), std::cend(layout),
                    std::back_inserter(attributeDescriptions), [binding] (auto &&description)
    {
        auto format = std::visit([normalized = description.normalized] (auto &&attribute)
        {
            using T = std::decay_t<decltype(attribute)>;
            return getFormat<T::number, typename T::type>(normalized);

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

    inputBindingDescriptions.shrink_to_fit();
    attributeDescriptions.shrink_to_fit();
}

#if OBSOLETE
template<class T>
bool VertexInputStateInfo::operator== (T &&rhs) const noexcept
{
    auto b1 = std::equal(std::cbegin(inputBindingDescriptions), std::cend(inputBindingDescriptions),
                            std::cbegin(rhs.inputBindingDescriptions));

    auto b2 = std::equal(std::cbegin(attributeDescriptions), std::cend(attributeDescriptions),
                            std::cbegin(rhs.attributeDescriptions));

    return b1 && b2;
}

template<class T>
std::size_t VertexInputStateInfo::hash_value() const noexcept
{
    std::size_t seed = 0;

    for (auto &&description : inputBindingDescriptions) {
        boost::hash_combine(seed, description.binding);
        boost::hash_combine(seed, description.stride);
        boost::hash_combine(seed, description.inputRate);
    }

    for (auto &&description : attributeDescriptions) {
        boost::hash_combine(seed, description.location);
        boost::hash_combine(seed, description.binding);
        boost::hash_combine(seed, description.format);
        boost::hash_combine(seed, description.offset);
    }

    return seed;
}
#endif