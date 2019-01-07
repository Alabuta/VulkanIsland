#pragma once

#include "vertexFormat.hxx"

namespace glTF
{
bool load(std::string_view name, staging::scene_t &scene);
}
