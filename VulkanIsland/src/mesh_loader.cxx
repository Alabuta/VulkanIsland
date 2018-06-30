#define _SCL_SECURE_NO_WARNINGS 

#include <sstream>
#include <regex>
#include <vector>
#include <variant>
#include <optional>
#include <tuple>
#include <map>
#include <functional>


#include "nlohmann/json.hpp"

#include "helpers.hxx"
#include "mesh_loader.hxx"

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
auto constexpr kBYTE                 = 0x1400; // 5120
auto constexpr kUNSIGNED_BYTE        = 0x1401;
auto constexpr kSHORT                = 0x1402;
auto constexpr kUNSIGNED_SHORT       = 0x1403; // 5123
auto constexpr kINT                  = 0x1404;
auto constexpr kUNSIGNED_INT         = 0x1405;
auto constexpr kFLOAT                = 0x1406; // 5126

//auto constexpr kARRAY_BUFFER         = 0x8892;
//auto constexpr kELEMENT_ARRAY_BUFFER = 0x8893;

namespace attribute {
using attribute_t = std::variant<
    vec<1, std::int8_t>,
    vec<2, std::int8_t>,
    vec<3, std::int8_t>,
    vec<4, std::int8_t>,

    vec<1, std::uint8_t>,
    vec<2, std::uint8_t>,
    vec<3, std::uint8_t>,
    vec<4, std::uint8_t>,

    vec<1, std::int16_t>,
    vec<2, std::int16_t>,
    vec<3, std::int16_t>,
    vec<4, std::int16_t>,

    vec<1, std::uint16_t>,
    vec<2, std::uint16_t>,
    vec<3, std::uint16_t>,
    vec<4, std::uint16_t>,

    vec<1, std::int32_t>,
    vec<2, std::int32_t>,
    vec<3, std::int32_t>,
    vec<4, std::int32_t>,

    vec<1, std::uint32_t>,
    vec<2, std::uint32_t>,
    vec<3, std::uint32_t>,
    vec<4, std::uint32_t>,

    vec<1, std::float_t>,
    vec<2, std::float_t>,
    vec<3, std::float_t>,
    vec<4, std::float_t>
>;

using buffer_t = wrap_variant_by_vector<attribute_t>::type;


enum class eSEMANTIC : std::size_t {
    nPOSITION = 0,
    nNORMAL,
    nTEXCOORD_0,
    nTEXCOORD_1,
    nTANGENT,
    nCOLOR_0,
    nJOINTS_0,
    nWEIGHTS_0
};

namespace semantic {
template<eSEMANTIC S>
struct attribute {
    static auto constexpr semantic{S};

    auto constexpr operator< (attribute rhs) const noexcept
    {
        return semantic < rhs.semantic;
    }
};

struct position : attribute<eSEMANTIC::nPOSITION> {};
struct normal : attribute<eSEMANTIC::nNORMAL> {};
struct tex_coord_0 : attribute<eSEMANTIC::nTEXCOORD_0> {};
struct tex_coord_1 : attribute<eSEMANTIC::nTEXCOORD_1> {};
struct tangent : attribute<eSEMANTIC::nTANGENT> {};
struct color_0 : attribute<eSEMANTIC::nCOLOR_0> {};
struct joints_0 : attribute<eSEMANTIC::nJOINTS_0> {};
struct weights_0 : attribute<eSEMANTIC::nWEIGHTS_0> {};
}

using semantic_t = std::variant<
    semantic::position,
    semantic::normal,
    semantic::tex_coord_0,
    semantic::tex_coord_1,
    semantic::tangent,
    semantic::color_0,
    semantic::joints_0,
    semantic::weights_0
>;

using vertex_attribute_t = std::variant<
    std::pair<semantic::position, attribute_t>,
    std::pair<semantic::normal, attribute_t>,
    std::pair<semantic::tex_coord_0, attribute_t>,
    std::pair<semantic::tex_coord_1, attribute_t>,
    std::pair<semantic::tangent, attribute_t>,
    std::pair<semantic::color_0, attribute_t>,
    std::pair<semantic::joints_0, attribute_t>,
    std::pair<semantic::weights_0, attribute_t>
>;

using accessor_t = std::variant<
    std::pair<semantic::position, std::size_t>,
    std::pair<semantic::normal, std::size_t>,
    std::pair<semantic::tex_coord_0, std::size_t>,
    std::pair<semantic::tex_coord_1, std::size_t>,
    std::pair<semantic::tangent, std::size_t>,
    std::pair<semantic::color_0, std::size_t>,
    std::pair<semantic::joints_0, std::size_t>,
    std::pair<semantic::weights_0, std::size_t>
>;

using vertex_buffer_t = std::variant<
    std::pair<
        std::tuple<semantic::position>,
        std::vector<std::tuple<vec<3, std::float_t>>>
    >,
    std::pair<
        std::tuple<semantic::position, semantic::normal>,
        std::vector<std::tuple<vec<3, std::float_t>, vec<3, std::float_t>>>
    >,
    std::pair<
        std::tuple<semantic::position, semantic::tex_coord_0>,
        std::vector<std::tuple<vec<3, std::float_t>, vec<2, std::float_t>>>
    >,
    std::pair<
        std::tuple<semantic::position, semantic::normal, semantic::tex_coord_0>,
        std::vector<std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<2, std::float_t>>>
    >,
    std::pair<
        std::tuple<semantic::position, semantic::normal, semantic::tangent>,
        std::vector<std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<4, std::float_t>>>
    >,
    std::pair<
        std::tuple<semantic::position, semantic::normal, semantic::tex_coord_0, semantic::tangent>,
        std::vector<std::tuple<vec<3, std::float_t>, vec<3, std::float_t>, vec<2, std::float_t>, vec<4, std::float_t>>>
    >
>;

using vertexx_t = std::variant<
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>
    >,
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>,
        std::pair<semantic::normal, vec<3, std::float_t>>
    >,
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>,
        std::pair<semantic::normal, vec<3, std::float_t>>,
        std::pair<semantic::tex_coord_0, vec<2, std::float_t>>
    >,
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>,
        std::pair<semantic::tex_coord_0, vec<2, std::float_t>>
    >,
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>,
        std::pair<semantic::normal, vec<3, std::float_t>>,
        std::pair<semantic::tangent, vec<4, std::float_t>>
    >,
    std::tuple<
        std::pair<semantic::position, vec<3, std::float_t>>,
        std::pair<semantic::normal, vec<3, std::float_t>>,
        std::pair<semantic::tex_coord_0, vec<2, std::float_t>>,
        std::pair<semantic::tangent, vec<4, std::float_t>>
    >
>;

std::optional<semantic_t> get_semantic(std::string_view name)
{
    if (name == "POSITION"sv)
        return semantic::position{ };

    else if (name == "NORMAL"sv)
        return semantic::normal{ };

    else if (name == "TEXCOORD_0"sv)
        return semantic::tex_coord_0{ };

    else if (name == "TEXCOORD_1"sv)
        return semantic::tex_coord_1{ };

    else if (name == "TANGENT"sv)
        return semantic::tangent{ };

    else if (name == "COLOR_0"sv)
        return semantic::color_0{ };

    else if (name == "JOINTS_0"sv)
        return semantic::joints_0{ };

    else if (name == "WEIGHTS_0"sv)
        return semantic::weights_0{ };

    return { };
}



}

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
        std::optional<std::size_t> material;
        std::size_t indices;

        std::set<glTF::attribute::accessor_t> attributeAccessors;

        struct attributes_t {
            std::optional<std::size_t> position;
            std::optional<std::size_t> normal;
            std::optional<std::size_t> tangent;
            std::optional<std::size_t> texCoord0;
            std::optional<std::size_t> texCoord1;
            std::optional<std::size_t> color0;
            std::optional<std::size_t> joints0;
            std::optional<std::size_t> weights0;
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
        matrix = j.at("matrix"s).get<std::decay_t<decltype(matrix)>>();

        // :TODO:
        // If the determinant of the transform is a negative value,
        // the winding order of the mesh triangle faces should be reversed.
        // This supports negative scales for mirroring geometry.
        node.transform = mat4(std::move(matrix));
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


        node.transform = std::make_tuple(vec3{std::move(translation)}, quat{std::move(rotation)}, vec3{std::move(scale)});
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

    std::transform(std::cbegin(json), std::cend(json), std::back_inserter(mesh.primitives), [] (nlohmann::json const &json_primitive)
    {
        mesh_t::primitive_t primitive;

        if (json_primitive.count("material"s))
            primitive.material = json_primitive.at("material"s).get<decltype(mesh_t::primitive_t::material)::value_type>();

        primitive.indices = json_primitive.at("indices"s).get<decltype(mesh_t::primitive_t::indices)>();

        auto const json_attributes = json_primitive.at("attributes"s);

        for (auto it = std::begin(json_attributes); it != std::end(json_attributes); ++it) {
            if (auto semantic = attribute::get_semantic(it.key()); semantic) {
                auto index = it->get<std::size_t>();

                std::visit([index, &primitive] (auto semantic)
                {
                    attribute::accessor_t accessor = std::make_pair(semantic, index);

                    primitive.attributeAccessors.emplace(std::move(accessor));

                }, semantic.value());
            }
        }

        if (json_attributes.count("POSITION"s))
            primitive.attributes.position = json_attributes.at("POSITION"s).get<decltype(mesh_t::primitive_t::attributes_t::position)::value_type>();

        if (json_attributes.count("NORMAL"s))
            primitive.attributes.normal = json_attributes.at("NORMAL"s).get<decltype(mesh_t::primitive_t::attributes_t::normal)::value_type>();

        if (json_attributes.count("TANGENT"s))
            primitive.attributes.tangent = json_attributes.at("TANGENT"s).get<decltype(mesh_t::primitive_t::attributes_t::tangent)::value_type>();

        if (json_attributes.count("TEXCOORD_0"s))
            primitive.attributes.texCoord0 = json_attributes.at("TEXCOORD_0"s).get<decltype(mesh_t::primitive_t::attributes_t::texCoord0)::value_type>();

        if (json_attributes.count("TEXCOORD_1"s))
            primitive.attributes.texCoord1 = json_attributes.at("TEXCOORD_1"s).get<decltype(mesh_t::primitive_t::attributes_t::texCoord1)::value_type>();

        if (json_attributes.count("COLOR_0"s))
            primitive.attributes.color0 = json_attributes.at("COLOR_0"s).get<decltype(mesh_t::primitive_t::attributes_t::color0)::value_type>();

        if (json_attributes.count("JOINTS_0"s))
            primitive.attributes.joints0 = json_attributes.at("JOINTS_0"s).get<decltype(mesh_t::primitive_t::attributes_t::joints0)::value_type>();

        if (json_attributes.count("WEIGHTS_0"s))
            primitive.attributes.weights0 = json_attributes.at("WEIGHTS_0"s).get<decltype(mesh_t::primitive_t::attributes_t::weights0)::value_type>();

        if (json_primitive.count("mode"s))
            primitive.mode = json_primitive.at("mode"s).get<decltype(mesh_t::primitive_t::mode)>();

        else primitive.mode = 4;

        return primitive;
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

template<std::size_t N>
std::optional<attribute::buffer_t> instantiate_attribute_buffer(std::int32_t componentType)
{
    switch (componentType) {
        case glTF::kBYTE:
            return std::vector<glTF::vec<N, std::int8_t>>();

        case glTF::kUNSIGNED_BYTE:
            return std::vector<glTF::vec<N, std::uint8_t>>();

        case glTF::kSHORT:
            return std::vector<glTF::vec<N, std::int16_t>>();

        case glTF::kUNSIGNED_SHORT:
            return std::vector<glTF::vec<N, std::uint16_t>>();

        case glTF::kINT:
            return std::vector<glTF::vec<N, std::int32_t>>();

        case glTF::kUNSIGNED_INT:
            return std::vector<glTF::vec<N, std::uint32_t>>();

        case glTF::kFLOAT:
            return std::vector<glTF::vec<N, std::float_t>>();

        default:
            return { };
    }
}

std::optional<attribute::buffer_t> instantiate_attribute_buffer(std::string_view type, std::int32_t componentType)
{
    if (type == "SCALAR"sv)
        return glTF::instantiate_attribute_buffer<1>(componentType);

    else if (type == "VEC2"sv)
        return glTF::instantiate_attribute_buffer<2>(componentType);

    else if (type == "VEC3"sv)
        return glTF::instantiate_attribute_buffer<3>(componentType);

    else if (type == "VEC4"sv)
        return glTF::instantiate_attribute_buffer<4>(componentType);

    return std::nullopt;
}
}

bool LoadGLTF(std::string_view name, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &_indices)
{
    auto current_path = fs::current_path();

    fs::path contents{"contents"s};
    fs::path folder{std::data(name)};

    if (!fs::exists(current_path / contents))
        contents = current_path / fs::path{"../../VulkanIsland"s} / contents;

    folder = contents / folder;

    auto glTF_path = folder / fs::path{"scene.gltf"s};

    std::ifstream glTFFile(glTF_path.native(), std::ios::in);

    if (glTFFile.bad() || glTFFile.fail()) {
        std::cerr << "can't open file: "s << glTF_path << std::endl;
        return false;
    }

    nlohmann::json json;
    glTFFile >> json;

    auto scenes = json.at("scenes"s).get<std::vector<glTF::scene_t>>();
    auto nodes = json.at("nodes"s).get<std::vector<glTF::node_t>>();

    auto meshes = json.at("meshes"s).get<std::vector<glTF::mesh_t>>();

    auto buffers = json.at("buffers"s).get<std::vector<glTF::buffer_t>>();
    auto bufferViews = json.at("bufferViews"s).get<std::vector<glTF::buffer_view_t>>();
    auto accessors = json.at("accessors"s).get<std::vector<glTF::accessor_t>>();

#if TEMPORARILY_DISABLED
    auto images = json.at("images"s).get<std::vector<glTF::image_t>>();
    auto textures = json.at("textures"s).get<std::vector<glTF::texture_t>>();
    auto samplers = json.at("samplers"s).get<std::vector<glTF::sampler_t>>();

    auto materials = json.at("materials"s).get<std::vector<glTF::material_t>>();
#endif

#if TEMPORARILY_DISABLED
    auto cameras = json.at("cameras"s).get<std::vector<glTF::camera_t>>();
#endif

    std::vector<std::vector<std::byte>> binBuffers;
    binBuffers.reserve(std::size(buffers));

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

        binBuffers.emplace_back(std::move(byteCode));
    }

    std::vector<glTF::attribute::buffer_t> attributeBuffers;
    attributeBuffers.reserve(std::size(accessors));
    
    for (auto &&accessor : accessors) {
        if (auto buffer = glTF::instantiate_attribute_buffer(accessor.type, accessor.componentType); buffer) {
            auto &&bufferView = bufferViews.at(accessor.bufferView);
            auto &&binBuffer = binBuffers.at(bufferView.buffer);

            std::visit([&accessor, &bufferView, &binBuffer, &attributeBuffers] (auto &&buffer)
            {
                auto const size = sizeof(typename std::decay_t<decltype(buffer)>::value_type);

                std::size_t const srcBeginIndex = accessor.byteOffset + bufferView.byteOffset;
                std::size_t const srcEndIndex = srcBeginIndex + accessor.count * size;

                buffer.resize(accessor.count);

                if (bufferView.byteStride != 0)
                    for (std::size_t srcIndex = srcBeginIndex, dstIndex = 0u; srcIndex < srcEndIndex; srcIndex += bufferView.byteStride, ++dstIndex)
                        memmove(&buffer.at(dstIndex), &binBuffer.at(srcIndex), size);

                else memmove(std::data(buffer), &binBuffer.at(srcBeginIndex), srcEndIndex - srcBeginIndex);

                attributeBuffers.emplace_back(std::move(buffer));

            }, std::move(buffer.value()));
        }
    }

    glTF::attribute::vertex_buffer_t vertexBuffer;

    std::vector<glTF::attribute::vertex_attribute_t> vertexAttributes;

    for (auto &&mesh : meshes) {
        for (auto &&primitive : mesh.primitives) {
            std::visit([&_indices, offset = std::size(vertices)] (auto &&indices)
            {
                std::transform(std::begin(indices), std::end(indices), std::back_inserter(_indices), [offset] (auto index)
                {
                    return static_cast<std::uint32_t>(offset + index.array.at(0));
                });

            }, attributeBuffers.at(primitive.indices));

#if NOT_YET_IMPLEMENTED
            for (auto &&accessor : primitive.attributeAccessors) {
                std::visit([&vertexAttributes, &attributeBuffers] (auto accessor)
                {
                    auto [semantic, index] = accessor;

                    /*glTF::attribute::vertex_attribute_t pair = std::make_pair(semantic, std::move(attributeBuffers.at(index)));

                    vertexAttributes.push_back(std::move(pair));*/

                    /*std::visit([semantic = semantic, &vertexAttributes] (auto &&buffer)
                    {
                        glTF::attribute::vertex_attribute_t pair = std::make_pair(semantic, std::move(buffer));

                        vertexAttributes.push_back(std::move(pair));

                    }, attributeBuffers.at(index));*/

                }, accessor);
            }
#endif

            auto &&positions = std::get<std::vector<glTF::vec<3, std::float_t>>>(attributeBuffers.at(*primitive.attributes.position));
            auto &&normals = std::get<std::vector<glTF::vec<3, std::float_t>>>(attributeBuffers.at(*primitive.attributes.normal));
            auto &&uvs = std::get<std::vector<glTF::vec<2, std::float_t>>>(attributeBuffers.at(*primitive.attributes.texCoord0));
            //std::vector<glTF::vec<2, std::float_t>> uvs(normals.size());

            vertices.reserve(std::size(positions));

            for (auto i = 0u; i < std::size(positions); ++i)
                vertices.emplace_back(positions.at(i).array, normals.at(i).array, uvs.at(i).array);
        }
    }

    return true;
}
