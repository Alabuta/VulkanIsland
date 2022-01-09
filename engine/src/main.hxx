#pragma once

#include "../include/config.hxx"
//#include "loaders/scene_loader.hxx"

struct app_t;
struct xformat;

namespace vulkan {
    class device;
}

void create_graphics_command_buffers(app_t &app);
void create_frame_data(app_t &app);
void cleanup_frame_data(app_t &app);
void create_sync_objects(app_t &app);
void build_render_pipelines(app_t &app, xformat const &model_);
void recreate_swap_chain(app_t &app);
void update_descriptor_set(app_t &app, vulkan::device const &device);
void update_viewport_descriptor_buffer(app_t const &app);
