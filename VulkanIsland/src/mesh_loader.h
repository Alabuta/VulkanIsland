#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <optional>

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

#include "main.h"
#include "helpers.h"

struct face_t {
    std::optional<std::vector<std::size_t>> position_index;
    std::optional<std::vector<std::size_t>> normal_index;
    std::optional<std::vector<std::size_t>> uv_index;
};

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<std::vector<std::size_t>> &faces);


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

    auto current_path = fs::current_path();

    fs::path directory{"meshes"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    auto const path = directory / name;

    if (!LoadBinaryModel(path, vertices)) {
        if (LoadOBJ(path, positions, normals, uvs, faces)) {
            for (auto &&face : faces) {
                std::transform(face.begin(), face.end(), face.begin(), [] (auto &&a) { return a - 1; });

                for (auto it_index = face.cbegin(); it_index < face.cend(); std::advance(it_index, 1)) {
                    auto position = positions.at(*it_index);
                    auto uv = uvs.at(*++it_index);
                    auto normal = normals.at(*++it_index);

                    vertices.emplace_back(std::move(position), std::move(normal), std::move(uv));
                }
            }

            SaveBinaryModel(path, vertices);
        }

        else return false;
    }

    count = static_cast<std::decay_t<decltype(count)>>(std::size(vertices) / 3);

    return true;
}