#pragma once

#include "../include/config.hxx"

void cleanup_frame_data(struct app_t &app);
void create_sync_objects(app_t &app);
void recreate_swap_chain(app_t &app);
void update_viewport_descriptor_buffer(app_t const &app);
