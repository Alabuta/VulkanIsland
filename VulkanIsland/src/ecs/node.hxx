#pragma once

#include "main.hxx"
#include "utility/mpl.hxx"
#include "math.hxx"

#include "ecs.hxx"
#include "transform.hxx"


namespace ecs
{
struct node final {
    entity_type parent;
    std::uint32_t depth;

    std::string name;

    node() noexcept = default;

    node(entity_type parent, std::uint32_t depth, std::string_view name) noexcept : parent{parent}, depth{depth}, name{name} { }

    template<class T1, class T2> requires mpl::all_same<node, T1, T2>
    bool constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
    {
        if (lhs.depth == rhs.depth)
            return lhs.parent < rhs.parent;

        return lhs.depth < rhs.depth;
    }
};

class NodeSystem final : public System {
public:
    entity_registry &registry;

    NodeSystem(entity_registry &registry) noexcept : registry{registry}
    {
        root_ = registry.create();

        registry.assign<Transform>(root_, glm::mat4{1}, glm::mat4{1});
        attachNode(root_, root_, "root"sv);
    }

    ~NodeSystem() = default;

    entity_type root() const noexcept { return root_; }

    std::optional<node> attachNode(entity_type parent, entity_type entity, std::string_view name = ""sv);

    void update() override;

private:
    entity_type root_;
};
}
