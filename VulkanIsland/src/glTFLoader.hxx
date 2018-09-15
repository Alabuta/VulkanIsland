#pragma once

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

namespace glTF
{
bool LoadScene(std::string_view name, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &indices);
}