#pragma once

#include "entityx/entityx.hh"
namespace ex = entityx;

#include <entt/entity/registry.hpp>


namespace ecs
{
using entity_registry = typename entt::registry<>;
using entity_type = entity_registry::entity_type;

class System {
public:

    virtual void update() = 0;

    virtual ~System() = default;
};
}
