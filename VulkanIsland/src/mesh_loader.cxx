
#include <sstream>
#include <regex>
#include <vector>
#include <variant>

#include "../includes/nlohmann/json.hpp"

#include "helpers.h"
#include "mesh_loader.h"

bool LoadOBJ(fs::path const &path, std::vector<vec3> &positions, std::vector<vec3> &normals, std::vector<vec2> &uvs, std::vector<index_t> &indices)
{
    std::ifstream file(path.native(), std::ios::in);

    if (!file.is_open()) {
        std::cerr << "can't open file: "s << path << std::endl;
        return false;
    }

    std::string line;
    std::string attribute;

    float x, y, z;

    std::size_t inx;
    std::vector<std::size_t> inxs;

    std::regex const regex_pattern("^[ |\t]*f[ |\t]+(.*)[ |\t]+(.*)[ |\t]+(.*)"s, std::regex::optimize);

    while (std::getline(file, line)) {
        std::istringstream stream(std::data(line));

        stream >> attribute;

        if (attribute == "v"s) {
            stream >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }

        else if (attribute == "vn"s) {
            stream >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }

        else if (attribute == "vt"s) {
            stream >> x >> y;
            uvs.emplace_back(x, y);
        }

        else if (attribute == "f"s) {
            std::smatch matches;
            if (std::regex_search(line, matches, regex_pattern)) {
                if (matches.size() != 3 + 1)
                    continue;

                for (auto index : { 1u, 2u, 3u }) {
                    std::istringstream in(matches.str(index));

                    inxs.clear();

                    bool b = true;

                    while (b) {
                        in >> inx;

                        if (in.eof() || in.bad())
                            b = false;

                        else if (in.fail()) {
                            in.clear();
                            in.ignore(std::numeric_limits<std::streamsize>::max(), '/');
                            continue;
                        }

                        inxs.emplace_back(inx - 1);
                    }

                    indices.emplace_back(index_t{ inxs.at(0), inxs.at(2), inxs.at(1) });
                }
            }
        }
    }

    file.close();

    return true;
}

namespace glTF {

/*auto constexpr GL_BYTE                 = 0x1400;
auto constexpr GL_UNSIGNED_BYTE        = 0x1401;
auto constexpr GL_SHORT                = 0x1402;
auto constexpr GL_UNSIGNED_SHORT       = 0x1403;
auto constexpr GL_INT                  = 0x1404;
auto constexpr GL_UNSIGNED_INT         = 0x1405;
auto constexpr GL_FLOAT                = 0x1406;

auto constexpr GL_ARRAY_BUFFER         = 0x8892;
auto constexpr GL_ELEMENT_ARRAY_BUFFER = 0x8893;*/

struct scene_t {
    std::string name;
    std::vector<std::size_t> nodes;
};

struct node_t {
    std::string name;

    std::variant<mat4, std::tuple<vec3, quat, vec3>> transform;

    std::size_t mesh, camera;

    std::vector<std::size_t> children;
};

struct buffer_t {
    std::size_t byteLength;
    std::string uri;
};

struct image_t {
    struct view_t {
        std::size_t bufferView;
        std::string mimeType;
    };

    std::variant<std::string, view_t> data;
};

struct mesh_t {
    struct primitive_t {
        std::size_t material;
        std::size_t indices;

        struct attributes_t {
            std::size_t position;
            std::size_t normal;
            std::size_t tangent;
            std::size_t texCoord0;
        } attributes;

        std::uint32_t mode;
    };

    std::vector<primitive_t> primitives;
};

struct material_t {
    struct pbr_t {
        struct texture_t {
            std::size_t index;
            std::size_t texCoord{0};
        };

        std::optional<texture_t> baseColorTexture;
        std::optional<texture_t> metallicRoughnessTexture;

        std::array<float, 4> baseColorFactor{{1.f, 1.f, 1.f, 1.f}};

        float metallicFactor, roughnessFactor;
    } pbr;

    struct normal_texture_t {
        std::size_t index;
        std::size_t texCoord{0};
        float scale;
    };

    std::optional<normal_texture_t> normalTexture;

    struct occlusion_texture_t {
        std::size_t index;
        std::size_t texCoord{0};
        float strength;
    };

    std::optional<occlusion_texture_t> occlusionTexture;

    struct emissive_texture_t {
        std::size_t index;
        std::size_t texCoord{0};
    };

    std::optional<emissive_texture_t> emissiveTexture;

    std::array<float, 3> emissiveFactor{{1.f, 1.f, 1.f}};

    std::string name;
    bool doubleSided{false};
};

struct camera_t {
    std::string type;

    struct perspective_t {
        float aspectRatio, yfov;
        float znear, zfar;
    };

    struct orthographic_t {
        float xmag, ymag;
        float znear, zfar;
    };

    std::variant<perspective_t, orthographic_t> instance;
};

struct texture_t {
    std::size_t source;
    std::size_t sampler;
};

struct sampler_t {
    std::uint32_t minFilter, magFilter;
    std::uint32_t wrapS, wrapT;
};

struct buffer_view_t {
    std::size_t buffer;
    std::size_t byteOffset;
    std::size_t byteLength;
    std::size_t byteStride;
    std::uint32_t target;
};

struct accessor_t {
    std::size_t bufferView;
    std::size_t byteOffset;
    std::size_t count;

    struct sparse_t {
        std::size_t count;
        std::size_t valuesBufferView;

        std::size_t indicesBufferView;
        std::uint32_t indicesComponentType;
    };

    std::optional<sparse_t> sparse;

    std::vector<float> min, max;

    std::uint32_t componentType;

    std::string type;
};

void from_json(nlohmann::json const &j, buffer_t &buffer)
{
    buffer.byteLength = j.at("byteLength"s).get<decltype(buffer_t::byteLength)>();
    buffer.uri = j.at("uri"s).get<decltype(buffer_t::uri)>();
}

void from_json(nlohmann::json const &j, image_t &image)
{
    if (j.count("uri"s))
        image.data = j.at("uri"s).get<std::string>();

    else image.data = image_t::view_t{
        j.at("bufferView"s).get<std::size_t>(),
        j.at("mimeType"s).get<std::string>()
    };
}

void from_json(nlohmann::json const &j, scene_t &scene)
{
    scene.name = j.at("name"s).get<std::decay_t<decltype(scene.name)>>();
    scene.nodes = j.at("nodes"s).get<std::decay_t<decltype(scene.nodes)>>();
}

void from_json(nlohmann::json const &j, node_t &node)
{
    node.name = j.at("name"s).get<std::decay_t<decltype(node.name)>>();

    if (j.count("matrix"s)) {
        std::array<float, 16> matrix;
        matrix = j.at("matrix"s).get<std::decay_t<decltype(matrix)>>();

        node.transform = std::move(mat4(std::move(matrix)));
    }

    else {
        std::array<float, 3> translation{{0.f, 0.f, 0.f}};
        std::array<float, 4> rotation{{0.f, 1.f, 0.f, 0.f}};
        std::array<float, 3> scale{{1.f, 1.f, 1.f}};

        if (j.count("translation"s))
            translation = j.at("translation"s).get<std::decay_t<decltype(translation)>>();

        if (j.count("rotation"s))
            rotation = j.at("rotation"s).get<std::decay_t<decltype(rotation)>>();

        if (j.count("scale"s))
            scale = j.at("scale"s).get<std::decay_t<decltype(scale)>>();


        node.transform = std::move(std::make_tuple(vec3{std::move(translation)}, quat{std::move(rotation)}, vec3{std::move(scale)}));
    }

    if (j.count("children"s))
        node.children = j.at("children"s).get<std::decay_t<decltype(node.children)>>();

    if (j.count("mesh"s))
        node.mesh = j.at("mesh"s).get<std::decay_t<decltype(node.mesh)>>();

    if (j.count("camera"s))
        node.camera = j.at("camera"s).get<std::decay_t<decltype(node.camera)>>();
}

void from_json(nlohmann::json const &j, mesh_t &mesh)
{
    auto const json = j.at("primitives"s);

    std::transform(std::cbegin(json), std::cend(json), std::back_inserter(mesh.primitives), [] (nlohmann::json const &primitive)
    {
        mesh_t::primitive_t mesh;

        mesh.material = primitive.at("material"s).get<decltype(mesh_t::primitive_t::material)>();
        mesh.indices = primitive.at("indices"s).get<decltype(mesh_t::primitive_t::indices)>();

        auto const json_attributes = primitive.at("attributes"s);

        mesh.attributes.position = json_attributes.at("POSITION"s).get<decltype(mesh_t::primitive_t::attributes_t::position)>();
        mesh.attributes.normal = json_attributes.at("NORMAL"s).get<decltype(mesh_t::primitive_t::attributes_t::normal)>();
        mesh.attributes.tangent = json_attributes.at("TANGENT"s).get<decltype(mesh_t::primitive_t::attributes_t::tangent)>();
        mesh.attributes.texCoord0 = json_attributes.at("TEXCOORD_0"s).get<decltype(mesh_t::primitive_t::attributes_t::texCoord0)>();

        mesh.mode = primitive.at("mode"s).get<decltype(mesh_t::primitive_t::mode)>();

        return mesh;
    });
}

void from_json(nlohmann::json const &j, material_t &material)
{
    auto const json_pbrMetallicRoughness = j.at("pbrMetallicRoughness"s);

    if (json_pbrMetallicRoughness.count("baseColorTexture"s)) {
        auto const json_baseColorTexture = json_pbrMetallicRoughness.at("baseColorTexture"s);

        material.pbr.baseColorTexture = {
            json_baseColorTexture.at("index"s).get<decltype(material_t::pbr_t::texture_t::index)>(),
            json_baseColorTexture.count("texCoord"s) ? json_baseColorTexture.at("texCoord"s).get<decltype(material_t::pbr_t::texture_t::texCoord)>() : 0
        };
    }

    material.pbr.baseColorFactor = json_pbrMetallicRoughness.at("baseColorFactor"s).get<decltype(material_t::pbr_t::baseColorFactor)>();

    if (json_pbrMetallicRoughness.count("metallicRoughnessTexture"s)) {
        auto const json_metallicRoughnessTexture = json_pbrMetallicRoughness.at("metallicRoughnessTexture"s);

        material.pbr.metallicRoughnessTexture = {
            json_metallicRoughnessTexture.at("index"s).get<decltype(material_t::pbr_t::texture_t::index)>(),
            json_metallicRoughnessTexture.count("texCoord"s) ? json_metallicRoughnessTexture.at("texCoord"s).get<decltype(material_t::pbr_t::texture_t::texCoord)>() : 0
        };
    }

    material.pbr.metallicFactor = json_pbrMetallicRoughness.at("metallicFactor"s).get<decltype(material_t::pbr_t::metallicFactor)>();
    material.pbr.roughnessFactor = json_pbrMetallicRoughness.at("roughnessFactor"s).get<decltype(material_t::pbr_t::roughnessFactor)>();

    if (j.count("normalTexture"s)) {
        auto const json_normalTexture = j.at("normalTexture"s);

        material.normalTexture = {
            json_normalTexture.at("index"s).get<decltype(material_t::normal_texture_t::index)>(),
            json_normalTexture.count("texCoord"s) ? json_normalTexture.at("texCoord"s).get<decltype(material_t::normal_texture_t::texCoord)>() : 0,
            json_normalTexture.at("scale"s).get<decltype(material_t::normal_texture_t::scale)>()
        };
    }

    if (j.count("occlusionTexture"s)) {
        auto const json_occlusionTexture = j.at("occlusionTexture"s);

        material.occlusionTexture = {
            json_occlusionTexture.at("index"s).get<decltype(material_t::occlusion_texture_t::index)>(),
            json_occlusionTexture.count("texCoord"s) ? json_occlusionTexture.at("texCoord"s).get<decltype(material_t::occlusion_texture_t::texCoord)>() : 0,
            json_occlusionTexture.at("strength"s).get<decltype(material_t::occlusion_texture_t::strength)>()
        };
    }

    if (j.count("emissiveTexture"s)) {
        auto const json_emissiveTexture = j.at("emissiveTexture"s);

        material.emissiveTexture = {
            json_emissiveTexture.at("index"s).get<decltype(material_t::emissive_texture_t::index)>(),
            json_emissiveTexture.count("texCoord"s) ? json_emissiveTexture.at("texCoord"s).get<decltype(material_t::emissive_texture_t::texCoord)>() : 0
        };

        material.emissiveFactor = j.at("emissiveFactor"s).get<decltype(material_t::emissiveFactor)>();
    }

    material.name = j.at("name"s).get<decltype(material_t::name)>();
    material.doubleSided = j.at("doubleSided"s).get<decltype(material_t::doubleSided)>();
}

void from_json(nlohmann::json const &j, camera_t &camera)
{
    camera.type = j.at("type"s).get<decltype(camera_t::type)>();

    auto const json_camera = j.at(camera.type);

    if (camera.type == "perspective"s) {
        camera_t::perspective_t instance;

        instance.aspectRatio = json_camera.get<decltype(camera_t::perspective_t::aspectRatio)>();
        instance.yfov = json_camera.get<decltype(camera_t::perspective_t::yfov)>();
        instance.znear = json_camera.get<decltype(camera_t::perspective_t::znear)>();

        if (json_camera.count("zfar"s))
            instance.zfar = json_camera.get<decltype(camera_t::perspective_t::zfar)>();

        else instance.zfar = std::numeric_limits<float>::infinity();

        camera.instance = instance;
    }

    else {
        camera.instance = camera_t::orthographic_t{
            json_camera.get<decltype(camera_t::orthographic_t::xmag)>(),
            json_camera.get<decltype(camera_t::orthographic_t::ymag)>(),
            json_camera.get<decltype(camera_t::orthographic_t::znear)>(),
            json_camera.get<decltype(camera_t::orthographic_t::zfar)>()
        };
    }
}

void from_json(nlohmann::json const &j, texture_t &texture)
{
    texture.source = j.at("source"s).get<decltype(texture_t::source)>();
    texture.sampler = j.at("sampler"s).get<decltype(texture_t::sampler)>();
}

void from_json(nlohmann::json const &j, sampler_t &sampler)
{
    sampler.minFilter = j.at("minFilter"s).get<decltype(sampler_t::minFilter)>();
    sampler.magFilter = j.at("magFilter"s).get<decltype(sampler_t::magFilter)>();

    sampler.wrapS = j.at("wrapS"s).get<decltype(sampler_t::wrapS)>();
    sampler.wrapT = j.at("wrapT"s).get<decltype(sampler_t::wrapT)>();
}

void from_json(nlohmann::json const &j, buffer_view_t &bufferView)
{
    bufferView.buffer = j.at("buffer"s).get<decltype(buffer_view_t::buffer)>();
    bufferView.byteOffset = j.at("byteOffset"s).get<decltype(buffer_view_t::byteOffset)>();
    bufferView.byteLength = j.at("byteLength"s).get<decltype(buffer_view_t::byteLength)>();

    if (j.count("byteStride"s))
        bufferView.byteStride = j.at("byteStride"s).get<decltype(buffer_view_t::byteStride)>();

    else bufferView.byteStride = 0;

    if (j.count("target"s))
        bufferView.target = j.at("target"s).get<decltype(buffer_view_t::target)>();
}

void from_json(nlohmann::json const &j, accessor_t &accessor)
{
    accessor.bufferView = j.at("bufferView"s).get<decltype(accessor_t::bufferView)>();

    if (j.count("byteOffset"s))
        accessor.byteOffset = j.at("byteOffset"s).get<decltype(accessor_t::byteOffset)>();

    else accessor.byteOffset = 0;

    accessor.count = j.at("count"s).get<decltype(accessor_t::count)>();

    if (j.count("sparse"s)) {
        auto const json_sparse = j.at("sparse"s);

        accessor_t::sparse_t sparse;

        sparse.count = json_sparse.at("count"s).get<decltype(accessor_t::sparse_t::count)>();

        sparse.valuesBufferView = json_sparse.at("values"s).at("bufferView"s).get<decltype(accessor_t::sparse_t::valuesBufferView)>();

        sparse.indicesBufferView = json_sparse.at("indices"s).at("bufferView"s).get<decltype(accessor_t::sparse_t::indicesBufferView)>();
        sparse.indicesComponentType = json_sparse.at("indices"s).at("componentType"s).get<decltype(accessor_t::sparse_t::indicesComponentType)>();

        accessor.sparse = sparse;
    }

    accessor.min = j.at("min"s).get<decltype(accessor_t::min)>();
    accessor.max = j.at("max"s).get<decltype(accessor_t::max)>();

    accessor.type = j.at("type"s).get<decltype(accessor_t::type)>();
    accessor.componentType = j.at("componentType"s).get<decltype(accessor_t::componentType)>();
}

}

bool LoadGLTF(std::string_view name)
{
    auto current_path = fs::current_path();

    fs::path contents{"contents"s};
    fs::path folder{std::data(name)};

    if (!fs::exists(current_path / contents))
        contents = current_path / fs::path{"../../VulkanIsland"s} / contents;

    folder = contents / folder;

    auto glTF_path = folder / fs::path{"scene.gltf"s};

    std::ifstream glTF_file(glTF_path.native(), std::ios::in);

    if (!glTF_file.is_open()) {
        std::cerr << "can't open file: "s << glTF_path << std::endl;
        return false;
    }

    nlohmann::json json;
    glTF_file >> json;

    auto buffers = json.at("buffers"s).get<std::vector<glTF::buffer_t>>();
    auto images = json.at("images"s).get<std::vector<glTF::image_t>>();

    auto scenes = json.at("scenes"s).get<std::vector<glTF::scene_t>>();

    auto nodes = json.at("nodes"s).get<std::vector<glTF::node_t>>();

    auto materials = json.at("materials"s).get<std::vector<glTF::material_t>>();

    auto textures = json.at("textures"s).get<std::vector<glTF::texture_t>>();
    auto samplers = json.at("samplers"s).get<std::vector<glTF::sampler_t>>();

    auto meshes = json.at("meshes"s).get<std::vector<glTF::mesh_t>>();

    auto bufferViews = json.at("bufferViews"s).get<std::vector<glTF::buffer_view_t>>();
    auto accessors = json.at("accessors"s).get<std::vector<glTF::accessor_t>>();

    // auto cameras = json.at("cameras"s).get<std::vector<glTF::camera_t>>();

    std::vector<std::vector<std::byte>> bin_buffers;
    bin_buffers.reserve(std::size(buffers));

    for (auto &&buffer : buffers) {
        auto bin_path = folder / fs::path{buffer.uri};

        std::ifstream bin_file(bin_path.native(), std::ios::in | std::ios::binary);

        if (!bin_file.is_open()) {
            std::cerr << "can't open file: "s << bin_path << std::endl;
            return false;
        }

        std::vector<std::byte> byteCode(buffer.byteLength);

        if (!byteCode.empty())
            bin_file.read(reinterpret_cast<char *>(std::data(byteCode)), std::size(byteCode));

        bin_buffers.emplace_back(std::move(byteCode));
    }

    return true;
}
