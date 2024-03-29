#ifdef OBSOLETE
#include <optional>
#include <variant>
#include <set>

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <filesystem>
namespace fs = std::filesystem;

#include <nlohmann/json.hpp>

#include "math/math.hxx"
#include "loaders/scene_loader.hxx"
#include "loaders/loaderGLTF.hxx"

#include "graphics/graphics.hxx"

namespace glTF
{
enum struct  GL {
    BYTE = 0x1400, UNSIGNED_BYTE,
    SHORT, UNSIGNED_SHORT,
    INT, UNSIGNED_INT,
    FLOAT,

    ARRAY_BUFFER = 0x8892,
    ELEMENT_ARRAY_BUFFER
};


enum struct  PRIMITIVE_MODE {
    POINTS = 0,
    LINES, LINE_STRIP,
    TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN
};
}

namespace
{
using accessor_t = std::pair<semantics_t, std::size_t>;
using accessors_set_t = std::set<accessor_t>;

std::optional<semantics_t> get_semantic(std::string_view name)
{
    if (name == "POSITION"sv)
        return semantic2::position{ };

    else if (name == "NORMAL"sv)
        return semantic2::normal{ };

    else if (name == "TEXCOORD_0"sv)
        return semantic2::tex_coord_0{ };

    else if (name == "TEXCOORD_1"sv)
        return semantic2::tex_coord_1{ };

    else if (name == "TANGENT"sv)
        return semantic2::tangent{ };

    else if (name == "COLOR_0"sv)
        return semantic2::color_0{ };

    else if (name == "JOINTS_0"sv)
        return semantic2::joints_0{ };

    else if (name == "WEIGHTS_0"sv)
        return semantic2::weights_0{ };

    return { };
}
}

namespace glTF
{
std::optional<graphics::PRIMITIVE_TOPOLOGY> constexpr get_primitive_topology(glTF::PRIMITIVE_MODE mode)
{
    switch (mode) {
        case  glTF::PRIMITIVE_MODE::POINTS:
            return graphics::PRIMITIVE_TOPOLOGY::POINTS;

        case  glTF::PRIMITIVE_MODE::LINES:
            return graphics::PRIMITIVE_TOPOLOGY::LINES;

        case  glTF::PRIMITIVE_MODE::LINE_STRIP:
            return graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP;

        case  glTF::PRIMITIVE_MODE::TRIANGLES:
            return graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        case  glTF::PRIMITIVE_MODE::TRIANGLE_STRIP:
            return graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP;

        case  glTF::PRIMITIVE_MODE::TRIANGLE_FAN:
            return graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_FAN;

        default:
            return { };
    }
}

template<std::size_t N>
std::size_t constexpr attribute_size(glTF::GL componentType)
{
    switch (componentType) {
        case  glTF::GL::BYTE:
            return N * sizeof(std::int8_t);

        case  glTF::GL::UNSIGNED_BYTE:
            return N * sizeof(std::uint8_t);

        case  glTF::GL::SHORT:
            return N * sizeof(std::int16_t);

        case  glTF::GL::UNSIGNED_SHORT:
            return N * sizeof(std::uint16_t);

        case  glTF::GL::INT:
            return N * sizeof(std::int32_t);

        case  glTF::GL::UNSIGNED_INT:
            return N * sizeof(std::uint32_t);

        case  glTF::GL::FLOAT:
            return N * sizeof(float);

        default:
            return 0;
    }
}

std::size_t attribute_size(std::string_view type, glTF::GL componentType)
{
    if (type == "SCALAR"sv)
        return attribute_size<1>(componentType);

    else if (type == "VEC2"sv)
        return attribute_size<2>(componentType);

    else if (type == "VEC3"sv)
        return attribute_size<3>(componentType);

    else if (type == "VEC4"sv)
        return attribute_size<4>(componentType);

    return 0;
}

template<std::uint32_t N>
std::optional<attribute_t> constexpr instantiate_attribute(glTF::GL componentType)
{
    switch (componentType) {
        case  glTF::GL::BYTE:
            return vec<N, std::int8_t>{};

        case  glTF::GL::UNSIGNED_BYTE:
            return vec<N, std::uint8_t>{};

        case  glTF::GL::SHORT:
            return vec<N, std::int16_t>{};

        case  glTF::GL::UNSIGNED_SHORT:
            return vec<N, std::uint16_t>{};

        case  glTF::GL::INT:
            return vec<N, std::int32_t>{};

        case  glTF::GL::UNSIGNED_INT:
            return vec<N, std::uint32_t>{};

        case  glTF::GL::FLOAT:
            return vec<N, float>{};

        default:
            return { };
    }
}

std::optional<attribute_t> instantiate_attribute(std::string_view type, glTF::GL componentType)
{
    if (type == "SCALAR"sv)
        return glTF::instantiate_attribute<1>(componentType);

    else if (type == "VEC2"sv)
        return glTF::instantiate_attribute<2>(componentType);

    else if (type == "VEC3"sv)
        return glTF::instantiate_attribute<3>(componentType);

    else if (type == "VEC4"sv)
        return glTF::instantiate_attribute<4>(componentType);

    return { };
}

std::optional<indices2_t> instantiate_index(glTF::GL componentType)
{
    switch (componentType) {
        case  glTF::GL::UNSIGNED_SHORT:
            return std::uint16_t{};

        case  glTF::GL::UNSIGNED_INT:
            return std::uint32_t{};

        default:
            return { };
    }
}
}

namespace glTF
{
struct scene_t {
    std::string name;
    std::vector<std::size_t> nodes;
};

struct node_t {
    std::string name;

    std::variant<glm::mat4, std::tuple<glm::vec3, glm::quat, glm::vec3>> transform;

    std::optional<std::size_t> mesh;
    std::optional<std::size_t> camera;

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
        std::optional<std::size_t> material;
        std::optional<std::size_t> indices;

        std::vector<accessor_t> attributes;

        accessors_set_t attribute_accessors;
    #ifdef NOT_YET_IMPLEMENTED
        saa_t attribute_accessors2;
    #endif
        glTF::PRIMITIVE_MODE mode{glTF::PRIMITIVE_MODE::TRIANGLES};
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
    std::size_t byteOffset{0};
    std::size_t byteLength;
    std::optional<std::size_t> byteStride;
    std::uint32_t target;
};

struct accessor_t {
    std::size_t bufferView{0};
    std::size_t byteOffset{0};
    std::size_t count;

    struct sparse_t {
        std::size_t count;
        std::size_t valuesBufferView;

        std::size_t indicesBufferView;
        std::uint32_t indicesComponentType;
    };

    std::optional<sparse_t> sparse;

    std::vector<float> min, max;

    glTF::GL componentType;

    std::string type;

    bool normalized{false};
};
}

namespace glTF
{
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
    if (j.count("name"s))
        scene.name = j.at("name"s).get<decltype(scene_t::name)>();

    else scene.name = ""s;

    scene.nodes = j.at("nodes"s).get<decltype(scene_t::nodes)>();
}

void from_json(nlohmann::json const &j, node_t &node)
{
    if (j.count("name"s))
        node.name = j.at("name"s).get<decltype(node_t::name)>();

    else node.name = ""s;

    if (j.count("matrix"s)) {
        std::array<float, 16> matrix;
        matrix = j.at("matrix"s).get<std::remove_cvref_t<decltype(matrix)>>();

        // :TODO:
        // If the determinant of the transform is a negative value,
        // the winding order of the mesh triangle faces should be reversed.
        // This supports negative scales for mirroring geometry.
        node.transform = glm::make_mat4(std::data(matrix));
    }

    else {
        std::array<float, 3> translation{{0.f, 0.f, 0.f}};
        std::array<float, 4> rotation{{0.f, 1.f, 0.f, 0.f}};
        std::array<float, 3> scale{{1.f, 1.f, 1.f}};

        if (j.count("translation"s))
            translation = j.at("translation"s).get<std::remove_cvref_t<decltype(translation)>>();

        if (j.count("rotation"s))
            rotation = j.at("rotation"s).get<std::remove_cvref_t<decltype(rotation)>>();

        if (j.count("scale"s))
            scale = j.at("scale"s).get<std::remove_cvref_t<decltype(scale)>>();

        node.transform = std::make_tuple(
            glm::make_vec3(std::data(translation)), glm::make_quat(std::data(rotation)), glm::make_vec3(std::data(scale))
        );
    }

    if (j.count("children"s))
        node.children = j.at("children"s).get<std::remove_cvref_t<decltype(node.children)>>();

    if (j.count("mesh"s))
        node.mesh = j.at("mesh"s).get<std::remove_cvref_t<decltype(node.mesh)::value_type>>();

    if (j.count("camera"s))
        node.camera = j.at("camera"s).get<std::remove_cvref_t<decltype(node.camera)::value_type>>();
}

void from_json(nlohmann::json const &j, mesh_t &mesh)
{
    auto const json = j.at("primitives"s);

    std::transform(std::cbegin(json), std::cend(json), std::back_inserter(mesh.primitives), [] (nlohmann::json const &json_primitive)
    {
        mesh_t::primitive_t primitive;

        if (json_primitive.count("material"s))
            primitive.material = json_primitive.at("material"s).get<decltype(mesh_t::primitive_t::material)::value_type>();

        if (json_primitive.count("indices"s))
            primitive.indices = json_primitive.at("indices"s).get<decltype(mesh_t::primitive_t::indices)::value_type>();

        auto const json_attributes = json_primitive.at("attributes"s);

        for (auto it = std::begin(json_attributes); it != std::end(json_attributes); ++it) {
            if (auto semantic = get_semantic(it.key()); semantic) {
                auto index = it->get<std::size_t>();

                primitive.attribute_accessors.emplace(semantic.value(), index);
            }

            if (auto semantic = get_semantic(it.key()); semantic) {
                auto index = it->get<std::size_t>();

                primitive.attributes.emplace_back(semantic.value(), index);
            }
        }

        if (json_primitive.count("mode"s))
            primitive.mode = json_primitive.at("mode"s).get<decltype(mesh_t::primitive_t::mode)>();

        return primitive;
    });
}

void from_json(nlohmann::json const &j, material_t &material)
{
    auto &&json_pbrMetallicRoughness = j.at("pbrMetallicRoughness"s);

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

    auto &&json_camera = j.at(camera.type);

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

    if (j.count("byteOffset"s))
        bufferView.byteOffset = j.at("byteOffset"s).get<decltype(buffer_view_t::byteOffset)>();

    else bufferView.byteOffset = 0;

    bufferView.byteLength = j.at("byteLength"s).get<decltype(buffer_view_t::byteLength)>();

    if (j.count("byteStride"s))
        bufferView.byteStride = j.at("byteStride"s).get<decltype(buffer_view_t::byteStride)::value_type>();

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

    if (j.count("min"s)) accessor.min = j.at("min"s).get<decltype(accessor_t::min)>();
    if (j.count("max"s)) accessor.max = j.at("max"s).get<decltype(accessor_t::max)>();

    accessor.type = j.at("type"s).get<decltype(accessor_t::type)>();
    accessor.componentType = j.at("componentType"s).get<decltype(accessor_t::componentType)>();
}
}


#if 0
namespace
{
std::vector<SceneTree> initSceneTree(std::vector<glTF::scene_t> const &scenes, std::vector<glTF::node_t> const &nodes)
{
    std::vector<SceneTree> sceneTrees;

    std::transform(std::begin(scenes), std::end(scenes), std::back_inserter(sceneTrees), [&nodes] (auto &&scene)
    {
        SceneTree sceneTree{scene.name};

        auto parent = sceneTree.root();

        using handles_t = std::vector<NodeHandle>;

        handles_t handles;
        std::vector<handles_t> handlesStack;

        std::vector<std::vector<std::size_t>> indicesStack{scene.nodes};

        while (!indicesStack.empty()) {
            auto &&indices = indicesStack.back();

            if (indices.empty()) {
                handlesStack.pop_back();

                if (!handlesStack.empty() && !handlesStack.back().empty())
                    parent = handlesStack.back().back();

                indicesStack.pop_back();

                continue;
            }

            if (std::size(indicesStack) != std::size(handlesStack)) {
                for (auto index : indices) {
                    auto &&node = nodes.at(index);

                    // TODO: add mesh component
                    /*if (node.mesh)
                        continue;*/

                    if (auto handle = sceneTree.AttachNode(parent, node.name); handle) {
                        handles.emplace_back(*handle);

                        std::visit([&sceneTree, handle] (auto &&transform)
                        {
                            using T = std::remove_cvref_t<decltype(transform)>;

                            if constexpr (std::is_same_v<T, glm::mat4>)
                                sceneTree.AddComponent<Transform>(*handle, transform, glm::mat4{1.f});

                            else {
                                auto &&[position, rotation, scale] = transform;

                                auto matrix = glm::translate(glm::mat4{1.f}, position);
                                matrix = matrix * glm::mat4_cast(rotation);
                                matrix = glm::scale(matrix, scale);

                                sceneTree.AddComponent<Transform>(*handle, std::move(matrix), glm::mat4{1.f});
                            }

                        }, node.transform);
                    }
                }

                handlesStack.emplace_back(std::move(handles));
            }

            auto index = indices.back();
            indices.pop_back();

            auto &&node = nodes.at(index);

            if (!node.children.empty()) {
                indicesStack.emplace_back(node.children);

                parent = handlesStack.back().back();
            }

            handlesStack.back().pop_back();
        }

        sceneTree.Update();
        return sceneTree;
    });

    return sceneTrees;
}

void initNodeGraph(std::vector<glTF::scene_t> const &scenes, std::vector<glTF::node_t> const &nodes, ecs::NodeSystem &nodeSystem)
{
    auto &&registry = nodeSystem.registry;
    //auto &&rootNode = registry.get<ecs::node>(nodeSystem.root());

    for (auto &&scene : scenes) {
        auto parent = nodeSystem.root();

        using entities_t = std::vector<ecs::entity_type>;

        entities_t handles;
        std::vector<entities_t> handlesStack;

        std::vector<std::vector<std::size_t>> indicesStack{scene.nodes};

        while (!indicesStack.empty()) {
            auto &&indices = indicesStack.back();

            if (indices.empty()) {
                handlesStack.pop_back();

                if (!handlesStack.empty() && !handlesStack.back().empty())
                    parent = handlesStack.back().back();

                indicesStack.pop_back();

                continue;
            }

            if (std::size(indicesStack) != std::size(handlesStack)) {
                for (auto index : indices) {
                    auto &&node = nodes.at(index);

                    auto entity = registry.create();

                    if (auto nodeComponent = nodeSystem.attachNode(parent, entity, node.name); nodeComponent) {
                        handles.emplace_back(entity);

                        std::visit([&registry, entity] (auto &&transform)
                        {
                            using T = std::remove_cvref_t<decltype(transform)>;

                            if constexpr (std::is_same_v<T, glm::mat4>)
                                registry.replace<Transform>(entity, transform, glm::mat4{1});

                            else {
                                auto &&[position, rotation, scale] = transform;

                                auto matrix = glm::translate(glm::mat4{1.f}, position);
                                matrix = matrix * glm::mat4_cast(rotation);
                                matrix = glm::scale(matrix, scale);

                                registry.replace<Transform>(entity, std::move(matrix), glm::mat4{1});
                            }

                        }, node.transform);
                    }

                #ifdef NOT_YET_IMPLEMENTED
                    if (node.mesh) {
                        registry.assign<ecs::mesh>(entity);
                    }
                #endif
                }

                handlesStack.emplace_back(std::move(handles));
            }

            auto index = indices.back();
            indices.pop_back();

            auto &&node = nodes.at(index);

            if (!node.children.empty()) {
                indicesStack.emplace_back(node.children);

                parent = handlesStack.back().back();
            }

            handlesStack.back().pop_back();
        }
    }
}
}
#endif


namespace glTF
{
bool load(std::string_view name, staging::scene_t &scene, ecs::NodeSystem &)
{
    fs::path contents{"contents/scenes"s};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"s / contents;

    auto folder = contents / name;

    nlohmann::json json;

    {
        auto path = folder / "scene.gltf"s;

        std::ifstream file{path.native(), std::ios::in};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << '\n';
            return false;
        }

        file >> json;
    }

    auto meshes = json.at("meshes"s).get<std::vector<glTF::mesh_t>>();

    auto buffers = json.at("buffers"s).get<std::vector<glTF::buffer_t>>();
    auto bufferViews = json.at("bufferViews"s).get<std::vector<glTF::buffer_view_t>>();
    auto accessors = json.at("accessors"s).get<std::vector<glTF::accessor_t>>();

#ifdef TEMPORARILY_DISABLED
    auto images = ([&json]
    {
        if (json.count("images"s))
            return json.at("images"s).get<std::vector<glTF::image_t>>();

        return std::vector<glTF::image_t>{};
    })();

    auto textures = ([&json]
    {
        if (json.count("textures"s))
            return json.at("textures"s).get<std::vector<glTF::texture_t>>();

        return std::vector<glTF::texture_t>{};
    })();

    auto samplers = ([&json]
    {
        if (json.count("samplers"s))
            return json.at("samplers"s).get<std::vector<glTF::sampler_t>>();

        return std::vector<glTF::sampler_t>{};
    })();

    auto materials = json.at("materials"s).get<std::vector<glTF::material_t>>();

    auto cameras = ([&json]
    {
        if (json.count("cameras"s))
            return json.at("cameras"s).get<std::vector<glTF::camera_t>>();

        return std::vector<glTF::camera_t>{};
    })();
#endif

    std::vector<std::vector<std::byte>> binBuffers;
    binBuffers.reserve(std::size(buffers));

    for (auto &&buffer : buffers) {
        auto path = folder / fs::path{buffer.uri};

        std::ifstream file{path.native(), std::ios::in | std::ios::binary};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << '\n';
            return false;
        }

        std::vector<std::byte> byte_code(buffer.byteLength);

        if (!byte_code.empty())
            file.read(reinterpret_cast<char *>(std::data(byte_code)), static_cast<std::streamsize>(std::size(byte_code)));

        binBuffers.emplace_back(std::move(byte_code));
    }

    auto &&vertexBuffer = scene.vertexBuffer;
    auto &&indexBuffer = scene.indexBuffer;

    std::size_t vertexBufferWriteIndex = 0u;
    std::size_t indexBufferWriteIndex = 0u;

    for (auto &&mesh : meshes) {
        staging::mesh_t mesh_;

        for (auto &&primitive : mesh.primitives) {
            staging::submesh_t submesh;

            if (auto topology = get_primitive_topology(primitive.mode); topology)
                submesh.topology = *topology;

            else continue;

            if (primitive.indices) {
                auto &&accessor = accessors.at(primitive.indices.value());

                if (auto indexInstance = instantiate_index(accessor.componentType); indexInstance) {
                    auto &&bufferView = bufferViews.at(accessor.bufferView);
                    auto &&binBuffer = binBuffers.at(bufferView.buffer);

                    auto count = accessor.count;
                    auto byteStride = bufferView.byteStride;

                    auto indexTypeSize = std::visit([] (auto indexInstance)
                    {
                        return sizeof(std::remove_cvref_t<decltype(indexInstance)>);

                    }, *indexInstance);

                    submesh.indices.begin = indexBufferWriteIndex;
                    submesh.indices.end = submesh.indices.begin + count * indexTypeSize;

                    submesh.indices.count = static_cast<std::uint32_t>(count);
                    submesh.indices.type = *indexInstance;

                    std::size_t const readBeginIndex = accessor.byteOffset + bufferView.byteOffset;
                    std::size_t const readEndIndex = readBeginIndex + count * indexTypeSize;

                    indexBuffer.resize(indexBufferWriteIndex + count * indexTypeSize);

                    if (byteStride) {
                        std::size_t readIndexOffset = readBeginIndex, writeIndexOffset = indexBufferWriteIndex;

                        for (; readIndexOffset < readEndIndex; readIndexOffset += *byteStride, writeIndexOffset += indexTypeSize)
                            memcpy(&indexBuffer.at(writeIndexOffset), &binBuffer.at(readIndexOffset), indexTypeSize);
                    }

                    else memcpy(&indexBuffer.at(indexBufferWriteIndex), &binBuffer.at(readBeginIndex), readEndIndex - readBeginIndex);

                    indexBufferWriteIndex += count * indexTypeSize;
                }

                else continue;

            #if 0
                if (auto index_buffer = instantiate_index_buffer(accessor.componentType); index_buffer) {
                    auto &&bufferView = bufferViews.at(accessor.bufferView);
                    auto &&binBuffer = binBuffers.at(bufferView.buffer);

                    indices = std::move(index_buffer.value());

                    std::size_t const begin = accessor.byteOffset + bufferView.byteOffset;

                    std::visit([begin, count = accessor.count, byteStride = bufferView.byteStride, &binBuffer] (auto &&indices)
                    {
                        using T = std::remove_cvref_t<decltype(indices)>::value_type;

                        auto size = sizeof(T);
                        std::size_t const end = begin + count * size;

                        indices.resize(count);

                        if (byteStride) {
                            std::size_t src_index = begin, dst_index = 0u;

                            for (; src_index < end; src_index += *byteStride, ++dst_index)
                                memcpy(&indices.at(dst_index), &binBuffer.at(src_index), size);
                        }

                        else memcpy(std::data(indices), &binBuffer.at(begin), end - begin);

                    }, indices);
                }
            #endif
            }

            std::size_t vertexTypeSize = 0;
            std::size_t verticesCount = 0;

            for (auto &&attribute : primitive.attribute_accessors) {
                auto [semantic, accessorIndex] = attribute;

                auto &&accessor = accessors.at(accessorIndex);

                auto attributeSize = glTF::attribute_size(accessor.type, accessor.componentType);

                if (attributeSize == 0) {
                    std::cerr << "unsupported attribute type"s << '\n';
                    continue;
                }

                vertexTypeSize += attributeSize;
                verticesCount = std::max(verticesCount, accessor.count);
            }

            submesh.vertices.begin = vertexBufferWriteIndex;
            submesh.vertices.end = submesh.vertices.begin + verticesCount * vertexTypeSize;

            submesh.vertices.count = static_cast<std::uint32_t>(verticesCount);

            vertexBuffer.resize(vertexBufferWriteIndex + verticesCount * vertexTypeSize);

            std::size_t dstOffset = 0;

            auto &&vertices = submesh.vertices;

            vertex_layout_t layout;

            for (auto &&attribute : primitive.attribute_accessors) {
                auto [semantic, accessor_index] = attribute;

                auto &&accessor = accessors.at(accessor_index);
                auto &&bufferView = bufferViews.at(accessor.bufferView);
                auto &&binBuffer = binBuffers.at(bufferView.buffer);

                auto normalized = accessor.normalized;

                if (auto _attribute = glTF::instantiate_attribute(accessor.type, accessor.componentType); _attribute) {
                    auto attributeSize = std::visit([dstOffset, semantic = semantic, normalized, &layout] (auto &&attribute)
                    {
                        using A = std::remove_cvref_t<decltype(attribute)>;

                        layout.emplace_back(dstOffset, semantic, std::move(attribute), normalized);

                        return sizeof(A);

                    }, std::move(*_attribute));

                    /*auto it_vertexBuffer = std::find_if(std::begin(scene.vertexBuffers), std::end(scene.vertexBuffers), [&layout] (auto &&vertexBuffer) {
                        return layout == vertexBuffer.layout;
                    });

                    auto &&vertexBuffer = *it_vertexBuffer;
                    vertexBuffer.buffer.resize(vertexBuffer.offset + verticesCount * vertexTypeSize);*/

                    std::size_t const readBeginIndex = accessor.byteOffset + bufferView.byteOffset;
                    std::size_t const readEndIndex = readBeginIndex + accessor.count * attributeSize;
                    std::size_t const readStep = bufferView.byteStride ? *bufferView.byteStride : attributeSize;

                    auto srcIndex = readBeginIndex, dstIndex = dstOffset + vertexBufferWriteIndex;

                    for (; srcIndex < readEndIndex; srcIndex += readStep, dstIndex += vertexTypeSize)
                        std::uninitialized_copy_n(&binBuffer.at(srcIndex), attributeSize, &vertexBuffer.at(dstIndex));
                        //memcpy(&buffer.at(dstIndex), &binBuffer.at(srcIndex), attributeSize);

                    dstOffset += attributeSize;
                }
            }

            vertices.layout = std::move(layout);

            vertexBufferWriteIndex += verticesCount * vertexTypeSize;

            mesh_.submeshes.push_back(std::move(submesh));
        }

        scene.meshes.push_back(std::move(mesh_));
    }

    auto scenes = json.at("scenes"s).get<std::vector<glTF::scene_t>>();
    auto nodes = json.at("nodes"s).get<std::vector<glTF::node_t>>();

#if 0
    auto sceneTree = initSceneTree(scenes, nodes);

    initNodeGraph(scenes, nodes, nodeSystem);
#endif

    return true;
}
}
#endif
