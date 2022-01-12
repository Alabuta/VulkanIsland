#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <random>
#include <execution>
#include <unordered_map>
#include <span>
#include <ranges>
#include <cmath>
#include <chrono>

#include <boost/align/align.hpp>
#include <boost/align.hpp>
#include <fmt/format.h>

#include "primitives/primitives.hxx"

#include "camera/camera_controller.hxx"
#include "camera/camera.hxx"

#include "platform/input/input_manager.hxx"

#include "graphics/compatibility.hxx"
#include "graphics/render_pass.hxx"
#include "graphics/vertex.hxx"

#include "renderer/render_flow.hxx"
#include "renderer/material.hxx"

#include "graphics/pipeline_states.hxx"
#include "graphics/graphics_pipeline.hxx"
#include "graphics/graphics.hxx"

#include "loaders/scene_loader.hxx"
#include "loaders/image_loader.hxx"
#include "loaders/TARGA_loader.hxx"

#include "descriptor.hxx"

#include "resources/framebuffer.hxx"
#include "resources/sync_objects.hxx"
#include "resources/memory_manager.hxx"
#include "resources/resource_manager.hxx"
#include "resources/image.hxx"
#include "resources/buffer.hxx"

#include "renderer/command_buffer.hxx"
#include "renderer/swapchain.hxx"
#include "renderer/renderer.hxx"
#include "renderer/config.hxx"

#include "vulkan/device.hxx"
#include "vulkan/instance.hxx"

#include "math/pack-unpack.hxx"
#include "math/math.hxx"

#include "utility/exceptions.hxx"
#include "utility/helpers.hxx"
#include "utility/mpl.hxx"

#include "main.hxx"
#include "app.hxx"


namespace temp
{
    static glm::vec4 generate_color()
    {
        static std::random_device random_device;
        static std::mt19937 generator{random_device()};

        std::uniform_real_distribution<float> uniform_real_distribution{0.f, 1.f};

        return glm::vec4{
            uniform_real_distribution(generator),
            uniform_real_distribution(generator),
            uniform_real_distribution(generator),
            1.f
        };
    }

#if 0
    void add_teapot(app_t &app, xformat &model_, std::size_t vertex_layout_index, graphics::INDEX_TYPE index_type, std::size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        auto const color = generate_color();

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        primitives::teapot_create_info const create_info{
            vertex_layout, topology, index_type,
            1u, 2u,
            color
        };

        auto const index_count = primitives::calculate_teapot_indices_count(create_info);
        auto const index_size = graphics::size_bytes(index_type);

        auto const vertex_count = primitives::calculate_teapot_vertices_count(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const index_buffer_allocation_size = index_count * index_size;
        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "index buffer size "s << index_buffer_allocation_size << " vertex buffer size "s << vertex_buffer_allocation_size << std::endl;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        {
            auto vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

            if (index_type != graphics::INDEX_TYPE::UNDEFINED) {
                auto index_staging_buffer = app.resource_manager->create_staging_buffer(index_buffer_allocation_size);

                primitives::generate_teapot_indexed(create_info, vertex_staging_buffer->mapped_range(), index_staging_buffer->mapped_range());

                index_buffer = app.resource_manager->stage_index_data(index_type, index_staging_buffer, app.transfer_command_pool);
            }

            else primitives::generate_teapot(create_info, vertex_staging_buffer->mapped_range());

            vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);
        }

        {
            xformat::meshlet meshlet;

            meshlet.topology = topology;

            auto first_vertex = (vertex_buffer->offset_bytes() - vertex_buffer_allocation_size) / vertex_size;

            meshlet.vertex_buffer = vertex_buffer;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertex_count);
            meshlet.first_vertex = static_cast<std::uint32_t>(first_vertex);

            if (index_type != graphics::INDEX_TYPE::UNDEFINED) {
                auto first_index = (index_buffer->offset_bytes() - index_buffer_allocation_size) / index_size;

                meshlet.index_buffer = index_buffer;
                meshlet.index_count = static_cast<std::uint32_t>(index_count);
                meshlet.first_index = static_cast<std::uint32_t>(first_index);
            }

            meshlet.material_index = material_index;
            meshlet.instance_count = 1;
            meshlet.first_instance = 0;

            std::vector<std::size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }
    }
#endif

    struct meshlet_create_info final {
        graphics::PRIMITIVE_TOPOLOGY topology;
        graphics::INDEX_TYPE index_type;

        size_t material_index;

        uint32_t vertex_count, vertex_size;
        uint32_t index_count, index_size;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;
    };

    static xformat::meshlet create_meshlet(meshlet_create_info const &info)
    {
        xformat::meshlet meshlet;

        meshlet.topology = info.topology;

        auto const index_buffer_allocation_size = info.index_count * info.index_size;
        auto const vertex_buffer_allocation_size = info.vertex_count * info.vertex_size;
        std::cout << "index buffer size " << index_buffer_allocation_size << " vertex buffer size " << vertex_buffer_allocation_size << std::endl;

        auto first_vertex = (info.vertex_buffer->offset_bytes() - vertex_buffer_allocation_size) / info.vertex_size;

        meshlet.vertex_buffer = info.vertex_buffer;
        meshlet.vertex_count = info.vertex_count;
        meshlet.first_vertex = static_cast<uint32_t>(first_vertex);

        if (info.index_type != graphics::INDEX_TYPE::UNDEFINED) {
            auto first_index = (info.index_buffer->offset_bytes() - index_buffer_allocation_size) / info.index_size;

            meshlet.index_buffer = info.index_buffer;
            meshlet.index_count = info.index_count;
            meshlet.first_index = static_cast<uint32_t>(first_index);
        }

        meshlet.material_index = info.material_index;
        meshlet.instance_count = 1;
        meshlet.first_instance = 0;

        return meshlet;
    }

    static void add_box(app_t &app, xformat &model_, size_t vertex_layout_index, graphics::INDEX_TYPE index_type, size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        std::array<glm::vec4, 6> colors{};
        std::generate_n(std::begin(colors), std::size(colors), generate_color);

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        primitives::box_create_info const create_info{
            vertex_layout, topology, index_type,
            1.f, 2.f, 2.f, 3u, 7u, 5u,
            colors
        };

        auto const index_count = primitives::calculate_box_indices_number(create_info);
        auto const index_size = graphics::size_bytes(index_type);

        auto const vertex_count = primitives::calculate_box_vertices_count(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const index_buffer_allocation_size = index_count * index_size;
        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "index buffer size " << index_buffer_allocation_size << " vertex buffer size " << vertex_buffer_allocation_size << std::endl;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        {
            auto vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

            if (index_type != graphics::INDEX_TYPE::UNDEFINED) {
                auto index_staging_buffer = app.resource_manager->create_staging_buffer(index_buffer_allocation_size);

                primitives::generate_box_indexed(create_info, vertex_staging_buffer->mapped_range(), index_staging_buffer->mapped_range());

                index_buffer = app.resource_manager->stage_index_data(index_type, index_staging_buffer, app.transfer_command_pool);
            }

            else primitives::generate_box(create_info, vertex_staging_buffer->mapped_range());

            vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);
        }

        {
            auto meshlet = create_meshlet(meshlet_create_info{
                    topology, index_type,
                    material_index,
                    static_cast<uint32_t>(vertex_count), static_cast<uint32_t>(vertex_size),
                    static_cast<uint32_t>(index_count), static_cast<uint32_t>(index_size),
                    vertex_buffer,
                    index_buffer
            });

            std::vector<size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }
    }

    static void add_plane(app_t &app, xformat &model_, size_t vertex_layout_index, graphics::INDEX_TYPE index_type, size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        auto const color = generate_color();

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP;

        primitives::plane_create_info const create_info{
            vertex_layout, topology, index_type,
            1.f, 1.f, 1u, 1u
        };

        auto const index_count = primitives::calculate_plane_indices_count(create_info);
        auto const index_size = graphics::size_bytes(index_type);

        auto const vertex_count = primitives::calculate_plane_vertices_count(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const index_buffer_allocation_size = index_count * index_size;
        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "index buffer size " << index_buffer_allocation_size << " vertex buffer size " << vertex_buffer_allocation_size << std::endl;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        {
            auto vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

            if (index_type != graphics::INDEX_TYPE::UNDEFINED) {
                auto index_staging_buffer = app.resource_manager->create_staging_buffer(index_buffer_allocation_size);

                primitives::generate_plane_indexed(create_info, vertex_staging_buffer->mapped_range(), index_staging_buffer->mapped_range(), color);

                index_buffer = app.resource_manager->stage_index_data(index_type, index_staging_buffer, app.transfer_command_pool);
            }

            else primitives::generate_plane(create_info, vertex_staging_buffer->mapped_range(), color);

            vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);
        }

        {
            auto meshlet = create_meshlet(meshlet_create_info{
                    topology, index_type,
                    material_index,
                    static_cast<uint32_t>(vertex_count), static_cast<uint32_t>(vertex_size),
                    static_cast<uint32_t>(index_count), static_cast<uint32_t>(index_size),
                    vertex_buffer,
                    index_buffer
            });

            std::vector<size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }
    }

    [[maybe_unused]] static void add_icosahedron(app_t &app, xformat &model_, size_t vertex_layout_index, size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        primitives::icosahedron_create_info const create_info{
            vertex_layout, topology,
            generate_color(),
            .4f, 100u
        };

        auto const vertex_count = primitives::calculate_icosahedron_vertices_count(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "vertex buffer size " << vertex_buffer_allocation_size << std::endl;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;

        {
            auto vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

            primitives::generate_icosahedron(create_info, vertex_staging_buffer->mapped_range());

            vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);
        }

        {
            auto meshlet = create_meshlet(meshlet_create_info{
                    topology, graphics::INDEX_TYPE::UNDEFINED,
                    material_index,
                    static_cast<uint32_t>(vertex_count), static_cast<uint32_t>(vertex_size),
                    0u, 0u,
                    vertex_buffer,
                    nullptr
            });

            std::vector<size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }
    }

    static void add_sphere(app_t &app, xformat &model_, size_t vertex_layout_index, graphics::INDEX_TYPE index_type, size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        primitives::sphere_create_info const create_info{
            vertex_layout, topology, index_type,
            .64f, 64u, 64u,
            generate_color()
        };

        auto const index_count = primitives::calculate_sphere_indices_count(create_info);
        auto const index_size = graphics::size_bytes(index_type);

        auto const vertex_count = primitives::calculate_sphere_vertices_count(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const index_buffer_allocation_size = index_count * index_size;
        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "index buffer size " << index_buffer_allocation_size << " vertex buffer size " << vertex_buffer_allocation_size << std::endl;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        {
            auto vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

            if (index_type != graphics::INDEX_TYPE::UNDEFINED) {
                auto index_staging_buffer = app.resource_manager->create_staging_buffer(index_buffer_allocation_size);

                primitives::generate_sphere_indexed(create_info, vertex_staging_buffer->mapped_range(), index_staging_buffer->mapped_range());

                index_buffer = app.resource_manager->stage_index_data(index_type, index_staging_buffer, app.transfer_command_pool);
            }

            else primitives::generate_sphere(create_info, vertex_staging_buffer->mapped_range());

            vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);
        }

        {
            auto meshlet = create_meshlet(meshlet_create_info{
                    topology, index_type,
                    material_index,
                    static_cast<uint32_t>(vertex_count), static_cast<uint32_t>(vertex_size),
                    static_cast<uint32_t>(index_count), static_cast<uint32_t>(index_size),
                    vertex_buffer,
                    index_buffer
            });

            std::vector<size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }
    }

    static xformat populate(app_t &app)
    {
        xformat model_;

        model_.materials.push_back(xformat::material{0, "debug/color-debug-material"});
        model_.materials.push_back(xformat::material{1, "debug/color-debug-material"});
        model_.materials.push_back(xformat::material{0, "debug/normals-debug"});
        model_.materials.push_back(xformat::material{1, "debug/normals-debug"});
        model_.materials.push_back(xformat::material{0, "debug/texture-coordinate-debug"});
        model_.materials.push_back(xformat::material{0, "debug/solid-wireframe"});
        model_.materials.push_back(xformat::material{0, "debug/normal-vectors-debug-material"});
        model_.materials.push_back(xformat::material{0, "debug/texture-debug"});
        model_.materials.push_back(xformat::material{0, "lighting/blinn-phong-material"});

        model_.transforms.push_back(glm::translate(glm::mat4{1.f}, glm::vec3{0, -1, 0}));
        model_.transforms.push_back(
                glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{-.5, -1, +.5}), glm::radians(-90.f), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
                glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{+.5, -1, +.5}), glm::radians(-90.f), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(glm::translate(glm::mat4{1.f}, glm::vec3{0, 0, -2}));
        model_.transforms.push_back(glm::translate(glm::mat4{1.f}, glm::vec3{0, 0, 0}));
        model_.transforms.push_back(
                glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{0}), glm::radians(90.f), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
                glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{+1, 1, -1}), glm::radians(-90.f * 0), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
                glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{-1, 1, -1}), glm::radians(-90.f * 0), glm::vec3{1, 0, 0}));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
                vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
                vertex::SEMANTIC::NORMAL, graphics::FORMAT::RGB32_SFLOAT,
                vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
                vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
                vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
                vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
                vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
                vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
                vertex::SEMANTIC::NORMAL, graphics::FORMAT::RG16_SNORM,
                vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
                vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        uint32_t node_index = 0;

        if constexpr (true) {
            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_plane(app, model_, 0, graphics::INDEX_TYPE::UNDEFINED, 7);

            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_plane(app, model_, 1, graphics::INDEX_TYPE::UINT_16, 1);

            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_plane(app, model_, 2, graphics::INDEX_TYPE::UINT_16, 2);

#if TEMPORARILY_DISABLED
            if (false) {
                struct vertex_struct final {
                    std::array<boost::float32_t, 3> position;
                    std::array<std::uint16_t, 2> tex_coord;
                    std::array<std::uint8_t, 4> color;
                };

                auto const vertex_count = 6u;
                auto const vertex_size = sizeof(vertex_struct);

                auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
                std::cout << "index buffer size "s << 0 << " vertex buffer size "s << vertex_buffer_allocation_size << std::endl;

                auto const vertex_layout_index = 1u;
                auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

                auto const vertex_staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

                auto it_data = reinterpret_cast<vertex_struct *>(std::to_address(std::data(vertex_staging_buffer->mapped_range())));

                auto constexpr max_8ui = std::numeric_limits<std::uint8_t>::max();
                auto constexpr max_16ui = std::numeric_limits<std::uint16_t>::max();

                // Second triangle
                *it_data++ = vertex_struct{
                    {0.f, 0.f, 0.f}, {max_16ui / 2, max_16ui / 2}, {0, 0, 0, max_8ui}
                };

                *it_data++ = vertex_struct{
                    {1.f, 0.f, -1.f}, {max_16ui, max_16ui}, {max_8ui, 0, max_8ui, max_8ui}
                };

                *it_data++ = vertex_struct{
                    {0.f, 0.f, -1.f}, {max_16ui / 2, max_16ui}, {0, 0, max_8ui, max_8ui}
                };

                // Third triangle
                *it_data++ = vertex_struct{
                    {0.f, 0.f, 0.f}, {max_16ui / 2, max_16ui / 2}, {max_8ui, 0, max_8ui, max_8ui}
                };

                *it_data++ = vertex_struct{
                    {-1.f, 0.f, -1.f}, {0, max_16ui}, {0, max_8ui, max_8ui, max_8ui}
                };

                *it_data = vertex_struct{
                    {-1.f, 0.f, 0.f}, {0, max_16ui / 2}, {max_8ui, max_8ui, 0, max_8ui}
                };

                auto const vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);

                {
                    // Second triangle
                    model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});

                    xformat::meshlet meshlet;

                    meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

                    auto const first_vertex = (vertex_buffer->offset_bytes() - vertex_buffer_allocation_size) / vertex_size;

                    meshlet.vertex_buffer = vertex_buffer;
                    meshlet.vertex_count = 3;
                    meshlet.first_vertex = static_cast<std::uint32_t>(first_vertex);

                    meshlet.material_index = 0;
                    meshlet.instance_count = 1;
                    meshlet.first_instance = 0;

                    std::vector const meshlets{std::size(model_.meshlets)};
                    model_.meshes.push_back(xformat::mesh{meshlets});

                    model_.meshlets.push_back(std::move(meshlet));
                }

                {
                    // Third triangle
                    model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});

                    xformat::meshlet meshlet;

                    meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

                    auto const first_vertex = (vertex_buffer->offset_bytes() - vertex_buffer_allocation_size) / vertex_size + 3u;

                    meshlet.vertex_buffer = vertex_buffer;
                    meshlet.vertex_count = 3;
                    meshlet.first_vertex = static_cast<std::uint32_t>(first_vertex);

                    meshlet.material_index = 3;
                    meshlet.instance_count = 1;
                    meshlet.first_instance = 0;

                    std::vector const meshlets{std::size(model_.meshlets)};
                    model_.meshes.push_back(xformat::mesh{meshlets});

                    model_.meshlets.push_back(std::move(meshlet));
                }
            }
#endif
        }

        if constexpr (true) {
            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_box(app, model_, 2, graphics::INDEX_TYPE::UINT_16, 5);
        }

        /*if constexpr (false) {
            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_icosahedron(app, model_, 2, 7);
        }*/

        if constexpr (true) {
            model_.scene_nodes.push_back(xformat::scene_node{node_index++, std::size(model_.meshes)});
            add_sphere(app, model_, 2, graphics::INDEX_TYPE::UINT_16, 7);
        }

        return model_;
    }
}

app_t::app_t(platform::window &window)
{
    instance = std::make_unique<vulkan::instance>();

    platform_surface = instance->get_platform_surface(window);

    device = std::make_unique<vulkan::device>(*instance, platform_surface);

    renderer_config = render::adjust_renderer_config(device->device_limits());

    memory_manager = std::make_unique<resource::memory_manager>(*device);
    resource_manager = std::make_unique<resource::resource_manager>(*device, renderer_config, *memory_manager);

    shader_manager = std::make_unique<graphics::shader_manager>(*device);
    material_factory = std::make_unique<graphics::material_factory>();
    vertex_input_state_manager = std::make_unique<graphics::vertex_input_state_manager>();
    pipeline_factory = std::make_unique<graphics::pipeline_factory>(*device, renderer_config, *shader_manager);

    render_pass_manager = std::make_unique<graphics::render_pass_manager>(*device);

    descriptor_registry = std::make_unique<graphics::descriptor_registry>(*device);

    if (auto command_pool = create_command_pool(*device, device->transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); command_pool)
        transfer_command_pool = *command_pool;

    else throw graphics::exception("failed to transfer command pool"s);

    if (auto command_pool = create_command_pool(*device, device->graphics_queue, 0); command_pool)
        graphics_command_pool = *command_pool;

    else throw graphics::exception("failed to graphics command pool"s);

    create_frame_data(*this);

    if (auto descriptor_set_layout = create_view_resources_descriptor_set_layout(*device); !descriptor_set_layout)
        throw graphics::exception("failed to create the view resources descriptor set layout"s);

    else view_resources_descriptor_set_layout = descriptor_set_layout.value();

    if (auto descriptor_set_layout = create_object_resources_descriptor_set_layout(*device); !descriptor_set_layout)
        throw graphics::exception("failed to create the object resources descriptor set layout"s);

    else object_resources_descriptor_set_layout = descriptor_set_layout.value();

    if (auto descriptor_set_layout = create_image_resources_descriptor_set_layout(*device); !descriptor_set_layout)
        throw graphics::exception("failed to create the image resources descriptor set layout"s);

    else image_resources_descriptor_set_layout = descriptor_set_layout.value();

#if TEMPORARILY_DISABLED
    if (auto result = glTF::load(sceneName, scene, nodeSystem); !result)
        throw resource::exception("failed to load a mesh"s);
#endif

    auto descriptor_sets_layouts = std::array{view_resources_descriptor_set_layout, object_resources_descriptor_set_layout, image_resources_descriptor_set_layout};

    if (auto result = create_pipeline_layout(*device, descriptor_sets_layouts); !result)
        throw graphics::exception("failed to create the pipeline layout"s);

    else pipeline_layout = result.value();

    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = load_texture(*device, *resource_manager, "checker-map.png"sv, transfer_command_pool); !result)
        throw resource::exception("failed to load a texture"s);

    else texture = std::move(result);

    if (auto result = resource_manager->create_image_sampler(graphics::TEXTURE_FILTER::LINEAR, graphics::TEXTURE_FILTER::LINEAR, graphics::TEXTURE_MIPMAP_MODE::LINEAR, 0.f, 0.f); !result)
        throw resource::exception("failed to create a texture sampler"s);

    else texture->sampler = result;

    xmodel = temp::populate(*this);

    auto const min_offset_alignment = static_cast<std::size_t>(device->device_limits().min_storage_buffer_offset_alignment);
    auto const aligned_offset = boost::alignment::align_up(sizeof(per_object_t), min_offset_alignment);

    objects.resize(std::size(xmodel.scene_nodes));

    aligned_buffer_size = aligned_offset * std::size(objects);

    if (per_object_buffer = create_storage_buffer(*resource_manager, aligned_buffer_size); per_object_buffer) {
        auto &&buffer = *per_object_buffer;

        auto const offset = buffer.memory()->offset();
        auto const size = buffer.memory()->size();

        if (auto result = vkMapMemory(device->handle(), buffer.memory()->handle(), offset, size, 0, &ssbo_mapped_ptr); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to map per object uniform buffer memory: {0:#x}", result));
    }

    else throw graphics::exception("failed to init per object uniform buffer"s);

    if (per_camera_buffer = create_uniform_buffer(*resource_manager, sizeof(camera::data_t)); !per_camera_buffer)
        throw graphics::exception("failed to init per camera uniform buffer"s);

    if (per_viewport_buffer = create_uniform_buffer(*resource_manager, sizeof(per_viewport_t)); !per_viewport_buffer)
        throw graphics::exception("failed to init per viewport uniform buffer"s);

    if (auto result = create_descriptor_pool(*device); !result)
        throw graphics::exception("failed to create the descriptor pool"s);

    else descriptor_pool = result.value();

    if (auto descriptor_sets = create_descriptor_sets(*device, descriptor_pool, descriptor_sets_layouts); descriptor_sets.empty())
        throw graphics::exception("failed to create the descriptor pool"s);

    else {
        view_resources_descriptor_set = descriptor_sets.at(0);
        object_resources_descriptor_set = descriptor_sets.at(1);
        image_resources_descriptor_set = descriptor_sets.at(2);
    }

    per_viewport_data.rect = glm::ivec4{0, 0, width, height};
    update_viewport_descriptor_buffer(*this);

    update_descriptor_set(*this, *device);

    build_render_pipelines(*this, xmodel);

    create_graphics_command_buffers(*this);

    create_sync_objects(*this);
}

void app_t::clean_up()
{
    if (device == nullptr)
        return;

    vkDeviceWaitIdle(device->handle());

    draw_commands_holder.clear();

    cleanup_frame_data(*this);

    for (auto &&semaphore : image_available_semaphores)
        if (semaphore)
            semaphore.reset();

    for (auto &&semaphore : render_finished_semaphores)
        if (semaphore)
            semaphore.reset();

    for (auto &&fence : concurrent_frames_fences)
        if (fence)
            fence.reset();

    for (auto &&fence : busy_frames_fences)
        if (fence)
            fence.reset();

    pipeline_factory.reset();

    vertex_input_state_manager.reset();

    material_factory.reset();

    shader_manager.reset();

    if (pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device->handle(), pipeline_layout, nullptr);

    if (view_resources_descriptor_set_layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device->handle(), view_resources_descriptor_set_layout, nullptr);

    if (object_resources_descriptor_set_layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device->handle(), object_resources_descriptor_set_layout, nullptr);

    if (image_resources_descriptor_set_layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device->handle(), image_resources_descriptor_set_layout, nullptr);

    vkDestroyDescriptorPool(device->handle(), descriptor_pool, nullptr);

    descriptor_registry.reset();

    render_pass.reset();
    render_pass_manager.reset();

    texture.reset();

    if (ssbo_mapped_ptr)
        vkUnmapMemory(device->handle(), per_object_buffer->memory()->handle());

    per_camera_buffer.reset();
    per_object_buffer.reset();
    per_viewport_buffer.reset();

    if (transfer_command_pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device->handle(), transfer_command_pool, nullptr);

    if (graphics_command_pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device->handle(), graphics_command_pool, nullptr);

    xmodel.meshlets.clear();

    resource_manager.reset();
    memory_manager.reset();

    device.reset();
    instance.reset();
}

void app_t::on_resize(std::int32_t w, std::int32_t h)
{
    if (width == w && height == h)
        return;

    width = w;
    height = h;

    per_viewport_data.rect = glm::ivec4{0, 0, width, height};

    if (width < 1 || height < 1)
        return;

    resize_callback = [this]
    {
        recreate_swap_chain(*this);

        update_viewport_descriptor_buffer(*this);

        camera_->aspect = static_cast<float>(width) / static_cast<float>(height);
    };
}
