#pragma once

#include "vertexFormat.hxx"

namespace glTF
{
bool load(std::string_view name, vertex_buffer_t &vertices, index_buffer_t &indices);
}
