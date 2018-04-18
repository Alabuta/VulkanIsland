#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_set>

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

#include "main.h"
#include "helpers.h"

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
    std::size_t operator() (T &&index) const
    {
        using namespace std::string_literals;
        return hash<std::string>{}(std::to_string(index.p) + "-"s + std::to_string(index.n) + "-"s + std::to_string(index.t));
    }
};
}

struct face_t {
    std::array<index_t, 3> indices;
};

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<std::vector<std::size_t>> &, 
             std::vector<index_t> &indices);



template<typename T>
bool SaveBinaryModel(fs::path path, std::vector<T> const &vertices)
{
    using namespace std::string_literals;

    std::ofstream file((path += fs::path{".bin"s}).native(), std::ios::out | std::ios::trunc | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    auto count = static_cast<std::size_t>(std::size(vertices));
    file.write(reinterpret_cast<char const *>(&count), sizeof(count));

    file.write(reinterpret_cast<char const *>(std::data(vertices)), std::size(vertices) * sizeof(std::decay_t<T>));

    return true;
}

template<typename T>
bool LoadBinaryModel(fs::path path, std::vector<T> &vertices)
{
    using namespace std::string_literals;

    std::ifstream file((path += fs::path{".bin"s}).native(), std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    std::size_t count = 18348;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));

    if (count < 1 || count % 3 != 0)
        return false;

    vertices.resize(count);
    file.read(reinterpret_cast<char *>(std::data(vertices)), std::size(vertices) * sizeof(std::decay_t<T>));

    return true;
}

template<typename T>
bool LoadModel(std::string_view _name, std::uint32_t &count, std::vector<T> &vertices)
{
    using namespace std::string_literals;

    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;

    std::vector<std::vector<std::size_t>> faces;
    std::vector<index_t> indices;

    auto current_path = fs::current_path();

    fs::path directory{"meshes"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    auto const path = directory / name;

    if (!LoadBinaryModel(path, vertices)) {
        if (LoadOBJ(path, positions, normals, uvs, faces, indices)) {
            std::unordered_set<index_t> unique_indices{indices.cbegin(), indices.cend()};

            for (auto[p, n, t] : indices) {
                vertices.emplace_back(positions.at(p), normals.at(n), uvs.at(t));
            }

            //for (auto &&face : faces1) {
                // std::transform(std::begin(face), std::end(face), std::begin(face), [] (auto &&a) { return a - 1; });

                /*for (auto it_index = face.cbegin(); it_index < face.cend(); std::advance(it_index, 1)) {
                    auto position = positions.at(*it_index);
                    auto uv = uvs.at(*++it_index);
                    auto normal = normals.at(*++it_index);

                    vertex_buffer.emplace_back(std::move(position), std::move(normal), std::move(uv));
                }*/

            //SaveBinaryModel(path, vertex_buffer);
        }
        
        else return false;
    }

    count = static_cast<std::decay_t<decltype(count)>>(std::size(vertices) / 3);

    return true;
}