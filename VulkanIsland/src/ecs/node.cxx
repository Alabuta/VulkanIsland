#include "node.hxx"


namespace ecs
{
std::optional<node *const> NodeSystem::attachNode(entity_type parent, entity_type entity, [[maybe_unused]] std::string_view name)
{
    if (parent == entity)
        return &registry.assign<ecs::node>(entity, parent, 0, name);

    if (!registry.has<node>(parent))
        return { };

    if (!registry.has<Transform>(entity))
        registry.assign<Transform>(entity, glm::mat4{1}, glm::mat4{1});

    auto &&parentNode = registry.get<node>(parent);

    return &registry.assign<ecs::node>(entity, parent, parentNode.depth + 1, name);
}

void NodeSystem::update()
{
    auto view = registry.view<node, Transform>();

    view.each([this, &view] (auto &&node, auto &&transform)
    {
        // TODO: revalidate scene graph.
        if (!registry.valid(node.parent))
            return;

        auto &&parentTransform = view.get<Transform>(node.parent);

        auto &&[localMatrix, worldMatrix] = transform;

        worldMatrix = parentTransform.worldMatrix * localMatrix;
    });
}
}