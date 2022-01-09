#pragma once


#include <boost/align/align.hpp>
#include <boost/align.hpp>
#include <fmt/format.h>
#include <string_view>
#include <string>
#include <random>
#include <execution>
#include <unordered_map>
#include <span>
#include <ranges>
#include <cmath>
#include <chrono>
#include "primitives/primitives.hxx"
#include "camera/camera_controller.hxx"
#include "camera/camera.hxx"
#include "platform/input/input_manager.hxx"
#include "graphics/compatibility.hxx"
#include "renderer/render_flow.hxx"
#include "graphics/render_pass.hxx"
#include "graphics/vertex.hxx"
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


struct per_object_t final {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed and inversed upper left 3x3 sub-matrix of the xmodel(world)-view matrix.
};

struct per_viewport_t final {
    glm::ivec4 rect{0, 0, 1920, 1080};
    //glm::vec2 depth{0, 1};
};

struct app_t final {
    uint32_t width{1920u};
    uint32_t height{1080u};

    std::size_t current_frame_index = 0;

    renderer::config renderer_config;

    std::unique_ptr<vulkan::instance> instance;
    std::unique_ptr<vulkan::device> device;

    renderer::platform_surface platform_surface;
    std::unique_ptr<renderer::swapchain> swapchain;

    std::unique_ptr<resource::resource_manager> resource_manager;
    std::unique_ptr<resource::memory_manager> memory_manager;

    std::unique_ptr<graphics::vertex_input_state_manager> vertex_input_state_manager;
    std::unique_ptr<graphics::shader_manager> shader_manager;
    std::unique_ptr<graphics::material_factory> material_factory;
    std::unique_ptr<graphics::pipeline_factory> pipeline_factory;

    std::unique_ptr<graphics::descriptor_registry> descriptor_registry;

    std::shared_ptr<graphics::render_pass> render_pass;
    std::unique_ptr<graphics::render_pass_manager> render_pass_manager;

    std::vector<graphics::attachment> attachments;
    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;

    std::array<std::shared_ptr<resource::semaphore>, renderer::kCONCURRENTLY_PROCESSED_FRAMES> image_available_semaphores;
    std::array<std::shared_ptr<resource::semaphore>, renderer::kCONCURRENTLY_PROCESSED_FRAMES> render_finished_semaphores;

    std::array<std::shared_ptr<resource::fence>, renderer::kCONCURRENTLY_PROCESSED_FRAMES> concurrent_frames_fences;
    std::vector<std::shared_ptr<resource::fence>> busy_frames_fences;

    camera_system cameraSystem;
    std::shared_ptr<camera> camera_;

    std::unique_ptr<orbit_controller> camera_controller;

    std::vector<per_object_t> objects;

    per_viewport_t per_viewport_data;

    VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

    VkCommandPool graphics_command_pool{VK_NULL_HANDLE}, transfer_command_pool{VK_NULL_HANDLE};

    VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

    VkDescriptorSetLayout view_resources_descriptor_set_layout{VK_NULL_HANDLE};
    VkDescriptorSetLayout object_resources_descriptor_set_layout{VK_NULL_HANDLE};
    VkDescriptorSetLayout image_resources_descriptor_set_layout{VK_NULL_HANDLE};
    VkDescriptorSet view_resources_descriptor_set{VK_NULL_HANDLE};
    VkDescriptorSet object_resources_descriptor_set{VK_NULL_HANDLE};
    VkDescriptorSet image_resources_descriptor_set{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> command_buffers;

    std::shared_ptr<resource::buffer> per_object_buffer, per_camera_buffer, per_viewport_buffer;
    void *ssbo_mapped_ptr{nullptr};

    size_t aligned_buffer_size{0u};

    std::shared_ptr<resource::texture> texture;

    renderer::draw_commands_holder draw_commands_holder;

    std::function<void()> resize_callback{nullptr};

    xformat xmodel;

    void init(platform::window &window);
    void clean_up();
};
