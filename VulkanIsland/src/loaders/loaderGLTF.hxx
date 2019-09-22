#pragma once


#if OBSOLETE
#include "ecs/node.hxx"
#include "ecs/transform.hxx"
#include "ecs/mesh.hxx"


namespace glTF
{
bool load(std::string_view name, staging::scene_t &scene, ecs::NodeSystem &nodeSystem);
}
#endif
