#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_set>

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

struct index_t {
    std::size_t p, n, t;

    template<class T, typename std::enable_if_t<std::is_same_v<index_t, std::decay_t<T>>>...>
    constexpr bool operator== (T &&rhs) const
    {
        return p == rhs.p && n == rhs.n && t == rhs.t;
    }
};

namespace std {
template<>
struct hash<index_t> {
    template<class T, typename std::enable_if_t<std::is_same_v<index_t, std::decay_t<T>>>...>
    constexpr std::size_t operator() (T &&index) const
    {
        using namespace std::string_literals;
        return hash<std::string>{}(std::to_string(index.p) + "-"s + std::to_string(index.n) + "-"s + std::to_string(index.t));
    }
};
}

namespace glTF {
template<std::size_t N, class T>
struct vec {
    std::array<T, N> array;
};

using vertex_t = std::variant<
    std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<2, std::float_t>, vec<4, std::float_t>>,
    std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<4, std::float_t>>
>;
}

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<index_t> &indices);

bool LoadGLTF(std::string_view name, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &indices);


template<typename T>
bool SaveBinaryModel(fs::path path, std::vector<T> const &vertices, std::vector<std::uint32_t> const &indices)
{
    using namespace std::string_literals;

    std::ofstream file((path += fs::path{".bin"s}).native(), std::ios::out | std::ios::trunc | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    auto vertices_count = static_cast<std::size_t>(std::size(vertices));
    file.write(reinterpret_cast<char const *>(&vertices_count), sizeof(vertices_count));

    file.write(reinterpret_cast<char const *>(std::data(vertices)), std::size(vertices) * sizeof(std::decay_t<T>));

    auto indices_count = static_cast<std::size_t>(std::size(indices));
    file.write(reinterpret_cast<char const *>(&indices_count), sizeof(indices_count));

    file.write(reinterpret_cast<char const *>(std::data(indices)), std::size(indices) * sizeof(std::decay_t<decltype(indices)>::value_type));

    return true;
}

template<typename T>
bool LoadBinaryModel(fs::path path, std::vector<T> &vertices, std::vector<std::uint32_t> &indices)
{
    using namespace std::string_literals;

    std::ifstream file((path += fs::path{".bin"s}).native(), std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    vertices.clear();
    indices.clear();

    std::size_t count = 18348;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));

    vertices.resize(count);
    file.read(reinterpret_cast<char *>(std::data(vertices)), std::size(vertices) * sizeof(std::decay_t<T>));

    file.read(reinterpret_cast<char *>(&count), sizeof(count));

    if (count < 1 || count % 3 != 0)
        return false;

    indices.resize(count);
    file.read(reinterpret_cast<char *>(std::data(indices)), std::size(indices) * sizeof(std::decay_t<decltype(indices)>::value_type));

    return true;
}

template<typename T>
bool LoadModel(std::string_view _name, std::vector<T> &vertices, std::vector<std::uint32_t> &indices)
{
    using namespace std::string_literals;

    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;

    std::vector<std::vector<std::size_t>> faces;
    std::vector<index_t> _indices;

    auto current_path = fs::current_path();

    fs::path directory{"meshes"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../VulkanIsland"s} / directory;

    auto const path = directory / name;

    if (!LoadBinaryModel(path, vertices, indices)) {
        if (LoadOBJ(path, positions, normals, uvs, _indices)) {
            vertices.clear();
            indices.clear();

            std::unordered_set<index_t> unique_indices{_indices.cbegin(), _indices.cend()};

            for (auto [p, n, t] : unique_indices)
                vertices.emplace_back(positions.at(p), normals.at(n), uvs.at(t));

            for (auto &&index : _indices) {
                auto const it = unique_indices.find(index);

                if (it == unique_indices.end())
                    return false;

                indices.emplace_back(static_cast<std::uint32_t>(std::distance(unique_indices.begin(), it)));
            }

            SaveBinaryModel(path, vertices, indices);
        }
        
        else return false;
    }

    return true;
}