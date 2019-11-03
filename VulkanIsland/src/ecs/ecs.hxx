#pragma once

//#include <entityx/entityx.h>
//namespace ex = entityx;

#include <entt/entt.hpp>
#include <entt/entity/registry.hpp>


namespace ecs
{
//using entity_registry = entt::registry;
//using entity_type = entity_registry::entity_type;

class System {
public:

    virtual void update() = 0;

    virtual ~System() = default;
};
}
