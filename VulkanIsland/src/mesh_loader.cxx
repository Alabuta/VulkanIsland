
#include <sstream>
#include <regex>
#include <vector>
#include <variant>
#include <json.hpp>

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
    std::string uri;
};

struct mesh_t {
    struct primitive_t {
        std::size_t material;
        std::size_t indices;

        struct attribute_t {
            ;
        } attribute;

        std::uint32_t mode;
    };
};

struct material_t {
    struct normalmap_t {
        std::size_t index;
        std::size_t texCoord;
        float scale;
    } normalmap;

    struct occlusionmap_t {
        std::size_t index;
        std::size_t texCoord;
        float scale;
    } occlusionmap;

    std::string name;
    bool doubleSide{false};
};

struct camera_t {
    std::string type;

    struct perspective_t {
        float aspectRatio, yfov, znear;
        float zfar = std::numeric_limits<float>::infinity();
    };

    struct orthographic_t {
        float xmag, ymag;
        float znear, zfar;
    };

    std::variant<perspective_t, orthographic_t> instance;
};


void from_json(nlohmann::json const &j, buffer_t &buffer)
{
    buffer.byteLength = j.at("byteLength"s).get<std::decay_t<decltype(buffer.byteLength)>>();
    buffer.uri = j.at("uri"s).get<std::decay_t<decltype(buffer.uri)>>();
}

void from_json(nlohmann::json const &j, image_t &image)
{
    image.uri = j.at("uri"s).get<std::decay_t<decltype(image.uri)>>();
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

void from_json(nlohmann::json const &j, camera_t &camera)
{
    camera.type = j.at("type"s).get<std::decay_t<decltype(camera.type)>>();

    auto const json_camera = j.at(camera.type);

    if (camera.type == "perspective"s) {
        camera_t::perspective_t instance;

        instance.aspectRatio = json_camera.get<decltype(instance.aspectRatio)>();
        instance.yfov = json_camera.get<decltype(instance.yfov)>();
        instance.znear = json_camera.get<decltype(instance.znear)>();

        if (json_camera.count("zfar"s))
            instance.zfar = json_camera.get<decltype(instance.zfar)>();

        camera.instance = instance;
    }

    else {
        camera_t::orthographic_t instance;

        instance.xmag = json_camera.get<decltype(instance.xmag)>();
        instance.ymag = json_camera.get<decltype(instance.ymag)>();
        instance.znear = json_camera.get<decltype(instance.znear)>();
        instance.zfar = json_camera.get<decltype(instance.zfar)>();

        camera.instance = instance;
    }
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

    // std::cout << std::setw(4) << json << '\n';

    auto buffers = json.at("buffers"s).get<std::vector<glTF::buffer_t>>();
    auto images = json.at("images"s).get<std::vector<glTF::image_t>>();

    auto scenes = json.at("scenes"s).get<std::vector<glTF::scene_t>>();

    auto nodes = json.at("nodes"s).get<std::vector<glTF::node_t>>();

    // auto cameras = json.at("cameras"s).get<std::vector<glTF::camera_t>>();

    return true;
}

/*
{
	"accessors": [
		{
			"bufferView": 2,
			"componentType": 5126,
			"count": 33576,
			"max": [
				56.88610076904297,
				64.0969009399414,
				358.36639404296875
			],
			"min": [
				-56.88610076904297,
				-64.09599304199219,
				-0.0681999996304512
			],
			"type": "VEC3"
		},
		{
			"bufferView": 2,
			"byteOffset": 402912,
			"componentType": 5126,
			"count": 33576,
			"max": [
				0.9999837875366211,
				0.9999515414237976,
				1
			],
			"min": [
				-0.9999638795852661,
				-0.999963104724884,
				-1
			],
			"type": "VEC3"
		},
		{
			"bufferView": 3,
			"componentType": 5126,
			"count": 33576,
			"max": [
				1,
				0.9999980926513672,
				0.9999934434890747,
				1
			],
			"min": [
				-0.9999961256980896,
				-0.9999960660934448,
				-0.9997523427009583,
				-1
			],
			"type": "VEC4"
		},
		{
			"bufferView": 1,
			"componentType": 5126,
			"count": 33576,
			"max": [
				3.906080961227417,
				3.8973329067230225
			],
			"min": [
				-2.906730890274048,
				-2.893332004547119
			],
			"type": "VEC2"
		},
		{
			"bufferView": 0,
			"componentType": 5125,
			"count": 178164,
			"max": [
				33575
			],
			"min": [
				0
			],
			"type": "SCALAR"
		}
	],
	"asset": {
		"extras": {
			"author": "auqolaq (https://sketchfab.com/auqolaq)",
			"license": "CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)",
			"source": "https://sketchfab.com/models/0219531e3c174e9dbf8c28f35290db5e",
			"title": "Hebe"
		},
		"generator": "Sketchfab-3.22.5",
		"version": "2.0"
	},
	"bufferViews": [
		{
			"buffer": 0,
			"byteLength": 712656,
			"byteOffset": 0,
			"name": "floatBufferViews",
			"target": 34963
		},
		{
			"buffer": 0,
			"byteLength": 268608,
			"byteOffset": 712656,
			"byteStride": 8,
			"name": "floatBufferViews",
			"target": 34962
		},
		{
			"buffer": 0,
			"byteLength": 805824,
			"byteOffset": 981264,
			"byteStride": 12,
			"name": "floatBufferViews",
			"target": 34962
		},
		{
			"buffer": 0,
			"byteLength": 537216,
			"byteOffset": 1787088,
			"byteStride": 16,
			"name": "floatBufferViews",
			"target": 34962
		}
	],
	"buffers": [
		{
			"byteLength": 2324304,
			"uri": "scene.bin"
		}
	],
	"images": [
		{
			"uri": "textures/HebehebemissinSG1_baseColor.png"
		},
		{
			"uri": "textures/HebehebemissinSG1_normal.png"
		},
		{
			"uri": "textures/HebehebemissinSG1_metallicRoughness.png"
		}
	],
	"materials": [
		{
			"doubleSided": true,
			"name": "HebehebemissinSG1",
			"normalTexture": {
				"index": 1,
				"scale": 1,
				"texCoord": 0
			},
			"occlusionTexture": {
				"index": 2,
				"strength": 1,
				"texCoord": 0
			},
			"pbrMetallicRoughness": {
				"baseColorFactor": [
					1,
					1,
					1,
					1
				],
				"baseColorTexture": {
					"index": 0,
					"texCoord": 0
				},
				"metallicFactor": 1,
				"metallicRoughnessTexture": {
					"index": 2,
					"texCoord": 0
				},
				"roughnessFactor": 1
			}
		}
	],
	"meshes": [
		{
			"primitives": [
				{
					"attributes": {
						"NORMAL": 1,
						"POSITION": 0,
						"TANGENT": 2,
						"TEXCOORD_0": 3
					},
					"indices": 4,
					"material": 0,
					"mode": 4
				}
			]
		}
	],
	"nodes": [
		{
			"children": [
				1
			],
			"name": "RootNode (gltf orientation matrix)",
			"rotation": [
				-0.7071067811865475,
				0,
				0,
				0.7071067811865476
			]
		},
		{
			"children": [
				2
			],
			"name": "RootNode (model correction matrix)"
		},
		{
			"children": [
				3
			],
			"name": "hebe.obj.cleaner.materialmerger.gles"
		},
		{
			"mesh": 0,
			"name": ""
		}
	],
	"samplers": [
		{
			"magFilter": 9729,
			"minFilter": 9987,
			"wrapS": 10497,
			"wrapT": 10497
		}
	],
	"scene": 0,
	"scenes": [
		{
			"name": "OSG_Scene",
			"nodes": [
				0
			]
		}
	],
	"textures": [
		{
			"sampler": 0,
			"source": 0
		},
		{
			"sampler": 0,
			"source": 1
		},
		{
			"sampler": 0,
			"source": 2
		}
	]
}
*/