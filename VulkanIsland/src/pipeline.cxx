#include <vector>
#include <iostream>
#include <algorithm>

#include <boost/functional/hash_fwd.hpp>

#include "pipeline.hxx"

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


VertexInputStateInfo::VertexInputStateInfo(vertex_layout_t const &layout) noexcept
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
std::size_t VertexInputStateInfo::hash_value(T &&rhs) const noexcept
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
