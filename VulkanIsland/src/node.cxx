#include "node.hxx"


namespace ecs
{
std::optional<Node *const> NodeSystem::attachNode(entity_type parent, entity_type entity, std::string_view name)
{
    if (parent == entity)
        return &registry.assign<ecs::Node>(entity, parent, 0);

    if (!registry.has<Node>(parent))
        return { };

    if (!registry.has<Transform>(entity))
        registry.assign<Transform>(entity, glm::mat4{1}, glm::mat4{1});

    auto &&parentNode = registry.get<Node>(parent);

    return &registry.assign<ecs::Node>(entity, parent, parentNode.depth + 1);
}

void NodeSystem::update()
{
    auto view = registry.view<Node, Transform>();

    view.each([this, &view] (auto &&node, auto &&transform)
    {
        // TODO: revalidate scene graph.
        if (!registry.valid(node.parent))
            return;

        auto &&parentTransform = view.get<Transform>(node.parent);

        auto &&[localMatrix, worldMatrix] = transform;

        localMatrix = worldMatrix * glm::inverse(parentTransform.worldMatrix);
    });
}
}