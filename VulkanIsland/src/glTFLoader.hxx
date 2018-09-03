#pragma once

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

namespace glTF
{
template<std::size_t N, class T>
struct vec {
    std::array<T, N> array;
};

using vertex_t = std::variant<
    std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<2, std::float_t>, vec<4, std::float_t>>,
    std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<4, std::float_t>>
>;
}

bool LoadGLTF(std::string_view name, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &indices);