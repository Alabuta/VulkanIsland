#ifdef _MSC_VER
#define USE_EXECUTION_POLICIES
#include <execution>
#endif

#include "sceneTree.hxx"


#if 0
std::optional<NodeHandle> SceneTree::AttachNode(NodeHandle parentHandle, std::string_view name)
{
    std::optional<NodeHandle> handle;

    if (!isNodeHandleValid(parentHandle))
        return { };

    auto parentNode = nodes.at(static_cast<std::size_t>(parentHandle));

    if (!isNodeValid(parentNode))
        return { };

    auto &&parentInfo = layers.at(parentNode.depth).at(parentNode.offset);
    auto &&parentChildren = parentInfo.children;

    auto const childrenDepth = parentNode.depth + 1;
    auto const childrenCount = parentChildren.end - parentChildren.begin;

    if (childrenDepth > std::numeric_limits<node_index_t>::max() - 1) {
        std::cerr << "requested scene tree depth is higher than maximum number\n"s;
        return { };
    }

    if (std::size(layers) < childrenDepth + 1)
        layers.resize(childrenDepth + 1);

    auto &&childrenLayer = layers.at(childrenDepth);

    if (std::size(layersChunks) < childrenDepth + 1)
        layersChunks.resize(childrenDepth + 1);

    auto &&layerChunks = layersChunks.at(childrenDepth);

    using difference_type_t = decltype(layersChunks)::difference_type;

    if (std::size(childrenLayer) == parentChildren.end) {
        auto index = parentChildren.end;

        handle.emplace(static_cast<NodeHandle>(std::size(nodes)));
        nodes.emplace_back(childrenDepth, index);

        childrenLayer.emplace_back(parentHandle, *handle, entities->create(), name);

        ++parentChildren.end;
    }

    else if (childrenCount > 0) {
        auto it_chunk = layerChunks.find(parentChildren.end);

        if (it_chunk != std::end(layerChunks)) {
            auto index = *it_chunk;

            handle.emplace(static_cast<NodeHandle>(std::size(nodes)));
            nodes.emplace_back(childrenDepth, index);

            auto it = std::next(std::begin(childrenLayer), static_cast<difference_type_t>(index));
            childrenLayer.emplace(it, parentHandle, *handle, entities->create(), name);

            ++parentChildren.end;

            layerChunks.erase(it_chunk);
        }

        else {
            auto FindGapInChunks = [] (auto begin, auto end) {
                return std::adjacent_find(begin, end, [] (auto lhs, auto rhs) {
                    return rhs - lhs != 1;
                });
            };

            auto it_range_begin = std::begin(layerChunks);
            auto it_range_end = std::end(layerChunks);

            auto const requestedSize = static_cast<difference_type_t>(childrenCount + 1);

            while (it_range_begin != std::end(layerChunks)) {
                it_range_end = FindGapInChunks(it_range_begin, std::end(layerChunks));

                if (it_range_end != std::end(layerChunks))
                    it_range_end = std::next(it_range_end);

                if (std::distance(it_range_begin, it_range_end) >= requestedSize)
                    break;

                it_range_begin = it_range_end;
            }

            difference_type_t new_begin_index = 0, new_node_index = 0;

            if (std::distance(it_range_begin, it_range_end) > 0) {
                auto it_range_edge = std::next(it_range_begin, requestedSize);

                new_begin_index = static_cast<difference_type_t>(*it_range_begin);
                new_node_index = static_cast<difference_type_t>(*std::prev(it_range_edge));

                layerChunks.erase(it_range_begin, it_range_edge);
            }

            else {
                new_begin_index = static_cast<difference_type_t>(std::size(childrenLayer));
                new_node_index = new_begin_index + requestedSize - 1;

                childrenLayer.resize(std::size(childrenLayer) + static_cast<std::size_t>(requestedSize));
            }

            auto const offset = new_begin_index - static_cast<difference_type_t>(parentChildren.begin);

            handle.emplace(static_cast<NodeHandle>(std::size(nodes)));
            nodes.emplace_back(childrenDepth, static_cast<std::size_t>(new_node_index));

            auto it_begin = std::next(std::begin(childrenLayer), static_cast<difference_type_t>(parentChildren.begin));
            auto it_end = std::next(std::begin(childrenLayer), static_cast<difference_type_t>(parentChildren.end));

            std::for_each(it_begin, it_end, [&nodes = nodes, offset] (auto &&nodeInfo)
            {
                auto &&node = nodes.at(static_cast<std::size_t>(nodeInfo.handle));

                node.offset += static_cast<std::size_t>(offset);
            });

            auto it_new_begin = std::next(std::begin(childrenLayer), new_begin_index);
            auto it_new_end = std::next(it_new_begin, requestedSize);

            std::vector<std::remove_cvref_t<decltype(layerChunks)>::value_type> newChunks(childrenCount);
            std::iota(std::begin(newChunks), std::end(newChunks), parentChildren.begin);

            parentChildren.begin = static_cast<std::size_t>(new_begin_index);
            parentChildren.end = parentChildren.begin + static_cast<std::size_t>(requestedSize);

            std::move(it_begin, it_end, it_new_begin);

            *std::prev(it_new_end) = NodeInfo{parentHandle, *handle, entities->create(), name};
            //childrenLayer.emplace(it_new_end, parentHandle, *handle, entityX->entities.create(), name);

            layerChunks.insert(std::begin(newChunks), std::end(newChunks));
            //std::move(std::begin(newChunks), std::end(newChunks), std::back_inserter(layerChunks));
        }
    }

    else {
        auto it_chunk = std::begin(layerChunks);

        if (it_chunk != std::end(layerChunks)) {
            auto index = *it_chunk;
            auto index_it_type = static_cast<difference_type_t>(index);

            handle.emplace(static_cast<NodeHandle>(std::size(nodes)));
            nodes.emplace_back(childrenDepth, index);

            auto it = std::next(std::begin(childrenLayer), index_it_type);
            childrenLayer.emplace(it, parentHandle, *handle, entities->create(), name);

            parentChildren.begin = index;
            parentChildren.end = parentChildren.begin + 1;

            layerChunks.erase(it_chunk);
        }

        else {
            handle.emplace(static_cast<NodeHandle>(std::size(nodes)));
            auto &&node = nodes.emplace_back(childrenDepth, std::size(childrenLayer));
            childrenLayer.emplace_back(parentHandle, *handle, entities->create(), name);

            parentChildren.begin = node.offset;
            parentChildren.end = parentChildren.begin + 1;
        }
    }

    return handle;
}

void SceneTree::RemoveNode(NodeHandle handle)
{
    if (!isNodeHandleValid(handle))
        return;

    auto &&node = nodes.at(static_cast<std::size_t>(handle));

    if (!isNodeValid(node))
        return;

    auto &&layer = layers.at(node.depth);
    auto &&info = layer.at(node.offset);
    auto &&children = info.children;

    auto const childrenCount = children.end - children.begin;

    auto &&parentHandle = info.parent;

    if (!isNodeHandleValid(parentHandle))
        return;

    auto parentNode = nodes.at(static_cast<std::size_t>(parentHandle));

    if (!isNodeValid(parentNode))
        return;

    auto &&parentInfo = layers.at(parentNode.depth).at(parentNode.offset);
    auto &&parentChildren = parentInfo.children;

    [[maybe_unused]] auto const parentChildrenDepth = parentNode.depth + 1;
    auto const parentChildrenCount = parentChildren.end - parentChildren.begin;

    if (parentChildrenCount > 1) {
        ;
    }

    else {
        parentChildren = { };
    }

    if (childrenCount > 0)
        DestroyChildren(handle);

    info.entity.destroy();

    auto &&layerChunks = layersChunks.at(node.depth);
    layerChunks.emplace(node.offset);

    node = { };
}

void SceneTree::DestroyChildren(NodeHandle handle)
{
    if (!isNodeHandleValid(handle))
        return;

    auto &&node = nodes.at(static_cast<std::size_t>(handle));

    if (!isNodeValid(node))
        return;

    auto &&layer = layers.at(node.depth);
    auto &&info = layer.at(node.offset);
    auto &&children = info.children;

    if (children.end - children.begin < 1)
        return;

    while (children.end - children.begin > 0) {
        [[maybe_unused]] auto depth = node.depth + 1;
        [[maybe_unused]] auto offset = children.end;
    }

    for (auto depth = node.depth + 1; depth < std::size(layers); ++depth) {
        ;
    }
}

void SceneTree::SetName(NodeHandle handle, std::string_view name)
{
    if (!isNodeHandleValid(handle))
        return;

    auto node = nodes.at(static_cast<std::size_t>(handle));

    if (!isNodeValid(node))
        return;

    auto &&info = layers.at(node.depth).at(node.offset);

    info.name = name;
}

void SceneTree::Update()
{
    // entities->for_each<Transform>([] (auto entity, auto &&transform) {
    //     std::cout << entity << "\n" << transform.localMatrix << "\n\n" << transform.worldMatrix << "\n\n";
    // });

    // std::cout << "#############################\n\n";

    auto find_sublings = [this] (auto begin, auto end)
    {
    #ifdef _MSC_VER
        return std::find_if(std::execution::par_unseq, begin, end, [this] (auto &&nodeInfo)
    #else
        return std::find_if(begin, end, [this] (auto &&nodeInfo)
    #endif
        {
            return isNodeHandleValid(nodeInfo.handle) && isNodeHandleValid(nodeInfo.parent);
        });
    };

    for (auto &&layer : layers) {
        auto it_sublings_begin = find_sublings(std::begin(layer), std::end(layer));

        while (it_sublings_begin != std::end(layer)) {
            auto parentTransformHandle = it_sublings_begin->entity.component<Transform>();

            auto it_sublings_end = std::adjacent_find(it_sublings_begin, std::end(layer), [] (auto &&lhs, auto &&rhs)
            {
                return lhs.parent != rhs.parent;
            });

            if (it_sublings_end != std::end(layer))
                it_sublings_end = std::next(it_sublings_end);

#ifdef _MSC_VER
            std::for_each(std::execution::par_unseq, it_sublings_begin, it_sublings_end, [parentTransformHandle] (auto &&nodeInfo)
#else
            std::for_each(it_sublings_begin, it_sublings_end, [parentTransformHandle] (auto &&nodeInfo)
#endif
            {
                auto transformHandle = nodeInfo.entity.template component<Transform>();
                transformHandle->worldMatrix = parentTransformHandle->worldMatrix * transformHandle->localMatrix;
            });

            it_sublings_begin = find_sublings(it_sublings_end, std::end(layer));
        }
    }

    // entities->for_each<Transform>([] (auto entity, auto &&transform) {
    //     std::cout << entity << "\n" << transform.localMatrix << "\n\n" << transform.worldMatrix << "\n\n";
    // });

    //entities->systems.update_all(0.f);
}
#endif
