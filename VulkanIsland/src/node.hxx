#pragma once

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

#include "ecs.hxx"
#include "transform.hxx"


namespace ecs
{
struct Node final {
    entity_type parent;
    std::uint32_t depth;

    Node() noexcept = default;

    Node(entity_type parent, std::uint32_t depth) noexcept : parent{parent}, depth{depth} { }
};

class NodeSystem final : public System {
public:

    NodeSystem(entity_registry &registry) noexcept : registry{registry} { }
    ~NodeSystem() = default;

    std::optional<Node *const> attachNode(entity_type parent, entity_type entity, std::string_view name = ""sv);

    void update() override;

private:
    entity_registry &registry;
};
}
