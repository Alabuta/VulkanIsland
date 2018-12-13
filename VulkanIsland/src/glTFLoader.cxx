#include <variant>

#include <filesystem>
namespace fs = std::filesystem;

#ifdef _MSC_VER
#pragma warning(push, 3)
#pragma warning(disable: 4127)
#include "nlohmann/json.hpp"
#pragma warning(pop)
#else
#include "nlohmann/json.hpp"
#endif

#include "glTFLoader.hxx"
#include "scene_tree.hxx"
#include "mesh.hxx"


namespace
{
enum class GL {
    BYTE = 0x1400, UNSIGNED_BYTE,
    SHORT, UNSIGNED_SHORT,
    INT, UNSIGNED_INT,
    FLOAT,

    ARRAY_BUFFER = 0x8892,
    ELEMENT_ARRAY_BUFFER
};
}

namespace glTF
{
namespace attribute
{
using attribute_t = std::variant <
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
> ;

using attribute_buffer_t = wrap_variant_by_vector<attribute_t>::type;


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



template<class S, class V>
struct expand;

template<class S, class... Ts>
struct expand<S, std::variant<Ts...>> {
    using type = std::tuple<std::pair<S, Ts>...>;
};


template<class... Ts>
using concat_tuples_types = decltype(std::tuple_cat(std::declval<Ts>()...));

template<class T>
struct tuple_to_variant;

template<class... Ts>
struct tuple_to_variant<std::tuple<Ts...>> {
    using type = std::variant<Ts...>;
};


using vertex_attribute1_t = tuple_to_variant<
    concat_tuples_types<
    expand<semantic::position, attribute_t>::type,
    expand<semantic::normal, attribute_t>::type,
    expand<semantic::tex_coord_0, attribute_t>::type,
    expand<semantic::tex_coord_1, attribute_t>::type,
    expand<semantic::tangent, attribute_t>::type,
    expand<semantic::color_0, attribute_t>::type,
    expand<semantic::joints_0, attribute_t>::type,
    expand<semantic::weights_0, attribute_t>::type
    >
>::type;

vertex_attribute1_t zzzz = std::pair<semantic::weights_0, vec<4, std::float_t>>{};

//static_assert(std::is_same_v<std::tuple_element_t<0, z>, std::pair<semantic::position, std::variant_alternative_t<0, attribute_t>>>, "!!!!");

using xxxxxxx = std::variant_alternative_t<7 * 28 + 27, vertex_attribute1_t>;
using yyyyyyy = std::pair<semantic::weights_0, vec<4, std::float_t>>;

static_assert(std::is_same_v<xxxxxxx, yyyyyyy>, "!!!!");



using accessor_t = std::pair<semantics_t, std::size_t>;

using accessors_set_t = std::set<accessor_t>;

template<std::size_t N, class V>
struct aggregation;

template<std::size_t N, class... Ts>
struct aggregation<N, std::variant<Ts...>> {
    using type = std::variant<std::tuple_element_t<N, Ts>...>;
};

using semantics_aggregated_t = aggregation<0, vertex_format_t>::type;
using types_aggregated_t = aggregation<1, vertex_format_t>::type;

using semantics_aggregated_buffer_t = wrap_variant_by_vector<semantics_aggregated_t>::type;
using types_aggregated_buffer_t = wrap_variant_by_vector<types_aggregated_t>::type;




std::optional<semantics_t> get_semantic(std::string_view name)
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

template<class VF, std::size_t I = 0>
constexpr auto get_vertex_format_index()
{
    using vf_t = std::variant_alternative_t<I, vertex_format_t>;

    if constexpr (std::is_same_v<VF, vf_t>)
        return I;

    else if constexpr (I + 1 < std::variant_size_v<vertex_format_t>)
        return get_vertex_format_index<VF, I + 1>();

    else return -1;
}



#if OBSOLETE
template<class T, class V, std::size_t I>
auto constexpr has_type_at_index = std::is_same_v<std::tuple_element_t<I, V>, T>;
#endif

//template<class S, class V, std::size_t I>
//auto constexpr has_semantic_at_index = has_type_at_index<S, std::tuple_element_t<0, V>, I>;
//
//static_assert(has_semantic_at_index<semantic::tex_coord_0, std::variant_alternative_t<3, vertex_format_t>, 2>, "tex_coord_0");



template<class T, class V, std::size_t O, typename = std::make_index_sequence<std::variant_size_v<V>>>
struct pick;

template<class T, class V, std::size_t O, std::size_t... I>
struct pick<T, V, O, std::index_sequence<I...>> {
    using candidates_t = std::tuple<std::variant_alternative_t<I, V>...>;
};

template<class S, class T>
struct tuple_has_type;

template<class S, class... Ts>
struct tuple_has_type<S, std::tuple<Ts...>> {
    static auto constexpr value = is_one_of_v<S, Ts...>;
};

template<class S, class V, std::size_t I>
auto constexpr has_semantic_at_index = tuple_has_type<S, std::tuple_element_t<0, std::variant_alternative_t<I, V>>>::value;

static_assert(has_semantic_at_index<semantic::tex_coord_0, vertex_format_t, 3>, "tex_coord_0");



template<class S, class V, std::size_t I = 0>
constexpr std::optional<std::size_t> get_semantic_index()
{
    if constexpr (has_semantic_at_index<S, V, I>)
        return I;

    else if constexpr (I + 1 < std::variant_size_v<V>)
        return get_semantic_index<S, V, I + 1>();

    return { };
}


std::optional<std::size_t> check(std::vector<semantics_t> const &semantics)
{
    for (auto semantic : semantics) {
        auto semanticIndex = std::visit([] (auto semantic) {
            using semantic_t = std::decay_t<decltype(semantic)>;

            if constexpr (!get_semantic_index<semantic_t, vertex_format_t>())
                std::cout << "!!!!\n";

            return get_semantic_index<semantic_t, vertex_format_t>();

        }, semantic);

        if (!semanticIndex)
            throw std::runtime_error("failed to match semantic type to vertex format.");

        else std::cout << *semanticIndex << '\n';
    }

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

    std::variant<mat4, std::tuple<vec3, quat, vec3>> transform;

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
        std::size_t indices;

        attribute::accessors_set_t attributeAccessors;

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

        std::uint32_t mode{4};
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

    GL componentType;

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
        node.mesh = j.at("mesh"s).get<std::decay_t<decltype(node.mesh)::value_type>>();

    if (j.count("camera"s))
        node.camera = j.at("camera"s).get<std::decay_t<decltype(node.camera)::value_type>>();
}

void from_json(nlohmann::json const &j, mesh_t &mesh)
{
    auto const json = j.at("primitives"s);

    std::transform(std::cbegin(json), std::cend(json), std::back_inserter(mesh.primitives), [] (nlohmann::json const &json_primitive) {
        mesh_t::primitive_t primitive;

        if (json_primitive.count("material"s))
            primitive.material = json_primitive.at("material"s).get<decltype(mesh_t::primitive_t::material)::value_type>();

        primitive.indices = json_primitive.at("indices"s).get<decltype(mesh_t::primitive_t::indices)>();

        auto const json_attributes = json_primitive.at("attributes"s);

        for (auto it = std::begin(json_attributes); it != std::end(json_attributes); ++it) {
            if (auto semantic = attribute::get_semantic(it.key()); semantic) {
                auto index = it->get<std::size_t>();

                primitive.attributeAccessors.emplace(semantic.value(), index);
            }
        }

        // TODO: remove
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

    if (j.count("min"s)) accessor.min = j.at("min"s).get<decltype(accessor_t::min)>();
    if (j.count("max"s)) accessor.max = j.at("max"s).get<decltype(accessor_t::max)>();

    accessor.type = j.at("type"s).get<decltype(accessor_t::type)>();
    accessor.componentType = j.at("componentType"s).get<decltype(accessor_t::componentType)>();
}

template<std::size_t N>
std::optional<attribute::attribute_buffer_t> instantiate_attribute_buffer(GL componentType)
{
    switch (componentType) {
        case GL::BYTE:
            return std::vector<vec<N, std::int8_t>>();

        case GL::UNSIGNED_BYTE:
            return std::vector<vec<N, std::uint8_t>>();

        case GL::SHORT:
            return std::vector<vec<N, std::int16_t>>();

        case GL::UNSIGNED_SHORT:
            return std::vector<vec<N, std::uint16_t>>();

        case GL::INT:
            return std::vector<vec<N, std::int32_t>>();

        case GL::UNSIGNED_INT:
            return std::vector<vec<N, std::uint32_t>>();

        case GL::FLOAT:
            return std::vector<vec<N, std::float_t>>();

        default:
            return { };
    }
}

std::optional<attribute::attribute_buffer_t> instantiate_attribute_buffer(std::string_view type, GL componentType)
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

template<std::size_t N>
std::optional<attribute::attribute_t> try_to_get_attribute_type(GL componentType)
{
    switch (componentType) {
        case GL::BYTE:
            return vec<N, std::int8_t>{};

        case GL::UNSIGNED_BYTE:
            return vec<N, std::uint8_t>{};

        case GL::SHORT:
            return vec<N, std::int16_t>{};

        case GL::UNSIGNED_SHORT:
            return vec<N, std::uint16_t>{};

        case GL::INT:
            return vec<N, std::int32_t>{};

        case GL::UNSIGNED_INT:
            return vec<N, std::uint32_t>{};

        case GL::FLOAT:
            return vec<N, std::float_t>{};

        default:
            return { };
    }
}

[[maybe_unused]] std::optional<attribute::attribute_t> try_to_get_attribute_type(std::string_view type, GL componentType)
{
    if (type == "SCALAR"sv)
        return glTF::try_to_get_attribute_type<1>(componentType);

    else if (type == "VEC2"sv)
        return glTF::try_to_get_attribute_type<2>(componentType);

    else if (type == "VEC3"sv)
        return glTF::try_to_get_attribute_type<3>(componentType);

    else if (type == "VEC4"sv)
        return glTF::try_to_get_attribute_type<4>(componentType);

    return std::nullopt;
}
}

namespace glTF
{
bool LoadScene(std::string_view name, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &_indices, [[maybe_unused]] index_buffer_t &inds)
{
    fs::path contents{"contents/scenes"sv};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"sv / contents;

    auto folder = contents / name;

    nlohmann::json json;

    {
        auto path = folder / "scene.gltf"sv;

        std::ifstream file{path.native(), std::ios::in};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << std::endl;
            return false;
        }

        file >> json;
    }

    auto scenes = json.at("scenes"s).get<std::vector<glTF::scene_t>>();
    auto nodes = json.at("nodes"s).get<std::vector<glTF::node_t>>();

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
                            using T = std::decay_t<decltype(transform)>;

                            if constexpr (std::is_same_v<T, mat4>)
                                sceneTree.AddComponent<Transform>(*handle, glm::make_mat4(std::data(transform.m)), glm::mat4{1.f});

                            else {
                                auto &&[position, rotation, scale] = transform;

                                auto matrix = glm::translate(glm::mat4{1.f}, glm::make_vec3(std::data(position.xyz)));
                                matrix = matrix * glm::mat4_cast(glm::make_quat(std::data(rotation.xyzw)));
                                matrix = glm::scale(matrix, glm::make_vec3(std::data(scale.xyz)));

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
        auto path = folder / buffer.uri;

        std::ifstream file{path.native(), std::ios::in | std::ios::binary};

        if (file.bad() || file.fail()) {
            std::cerr << "failed to open file: "s << path << std::endl;
            return false;
        }

        std::vector<std::byte> byteCode(buffer.byteLength);

        if (!byteCode.empty())
            file.read(reinterpret_cast<char *>(std::data(byteCode)), static_cast<std::int64_t>(std::size(byteCode)));

        binBuffers.emplace_back(std::move(byteCode));
    }

    std::vector<glTF::attribute::attribute_buffer_t> attributeBuffers;
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

    //vertex_buffer_t vertexBuffer;

#if NOT_YET_IMPLEMENTED
    std::vector<glTF::attribute::vertex_attribute_t> vertexAttributes;
#endif

    for (auto &&mesh : meshes) {
        for (auto &&primitive : mesh.primitives) {
            std::visit([&_indices, offset = std::size(vertices)] (auto &&indices)
            {
                std::transform(std::begin(indices), std::end(indices), std::back_inserter(_indices), [offset] (auto index)
                {
#if !defined( __clang__) && !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
                    return static_cast<std::uint32_t>(offset + static_cast<std::size_t>(index.array.at(0)));

#if !defined( __clang__) && !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
                });

            }, attributeBuffers.at(primitive.indices));

            auto &&attributeAccessors = primitive.attributeAccessors;

            std::vector<semantics_t> semantics;

            std::transform(std::begin(attributeAccessors), std::end(attributeAccessors), std::back_inserter(semantics), [] (auto accessor)
            {
                return accessor.first;
            });

#if 0
            attribute::check(semantics);

            std::vector<attribute::vertex_attribute_t> xxxx;

            for (auto &&attributeAccessor : primitive.attributeAccessors) {
                auto [semantic1, index] = attributeAccessor;

                std::visit([&attributeBuffers, &xxxx, index] (auto semantic1)
                {
                    //using semantic_t = decltype(semantic);

                    //semantics = std::tuple_cat(semantics, std::make_tuple(semantic));

                    std::visit([&xxxx, semantic = semantic1] (auto attributeBuffer)
                    {
                        using attribute_t = typename std::decay_t<decltype(attributeBuffer)>::value_type;

                        //using vf_t = std::pair<std::tuple<semantic_t>, std::tuple<attribute_t>>;

                        //xxxx.emplace_back(std::make_pair(semantic, attribute_t{}));

                        /*auto vertex_format_index = attribute::get_vertex_format_index<vf_t>();

                        if (vertex_format_index == -1)
                            throw std::runtime_error("unsupported vertex format"s);*/

                    }, attributeBuffers.at(index));

                    //[[maybe_unused]] auto &&accessor = accessors.at(index);

                    /*[[maybe_unused]] auto attribyte_type = std::get<>(glTF::try_to_get_attribute_type(accessor.type, accessor.componentType));

                    if (attribyte_type) {
                    }

                    else throw std::runtime_error("unsupported attribute type"s);*/

                }, semantic1);
            }
#endif

            //static_assert(attribute::is_vertex_format_v<std::pair<std::tuple<semantic::position, semantic::normal>, std::tuple<vec<3, std::float_t>, vec<2, std::float_t>>>>, "33333");

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

            auto &&positions = std::get<std::vector<vec<3, std::float_t>>>(attributeBuffers.at(*primitive.attributes.position));
            auto &&normals = std::get<std::vector<vec<3, std::float_t>>>(attributeBuffers.at(*primitive.attributes.normal));
            auto &&uvs = std::get<std::vector<vec<2, std::float_t>>>(attributeBuffers.at(*primitive.attributes.texCoord0));
            //std::vector<vec<2, std::float_t>> uvs(normals.size());

            vertices.reserve(std::size(positions));

            for (auto i = 0u; i < std::size(positions); ++i)
                vertices.emplace_back(positions.at(i).array, normals.at(i).array, uvs.at(i).array);
        }
    }

    return true;
}
}
