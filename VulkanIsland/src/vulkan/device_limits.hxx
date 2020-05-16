#pragma once

#include <cstdint>
#include <array>

#include "graphics/graphics.hxx"


namespace vulkan
{
#ifdef _MSC_VER
#pragma warning(error : 4820)
#endif
    struct device_limits final {
        std::uint64_t                     buffer_image_granularity;
        std::uint64_t                     sparse_address_space_size;
        std::uint64_t                     min_texel_buffer_offset_alignment;
        std::uint64_t                     min_uniform_buffer_offset_alignment;
        std::uint64_t                     min_storage_buffer_offset_alignment;
        std::uint64_t                     optimal_buffer_copy_offset_alignment;
        std::uint64_t                     optimal_buffer_copy_row_pitch_alignment;
        std::uint64_t                     non_coherent_atom_size;

        std::size_t                       min_memory_map_alignment;

        std::uint32_t                     max_image_dimension_1D;
        std::uint32_t                     max_image_dimension_2D;
        std::uint32_t                     max_image_dimension_3D;
        std::uint32_t                     max_image_dimension_cube;
        std::uint32_t                     max_image_array_layers;
        std::uint32_t                     max_texel_buffer_elements;
        std::uint32_t                     max_uniform_buffer_range;
        std::uint32_t                     max_storage_buffer_range;
        std::uint32_t                     max_push_constants_size;
        std::uint32_t                     max_memory_allocation_count;
        std::uint32_t                     max_sampler_allocation_count;
        std::uint32_t                     max_bound_descriptor_sets;
        std::uint32_t                     max_per_stage_descriptor_samplers;
        std::uint32_t                     max_per_stage_descriptor_uniform_buffers;
        std::uint32_t                     max_per_stage_descriptor_storage_buffers;
        std::uint32_t                     max_per_stage_descriptor_sampled_images;
        std::uint32_t                     max_per_stage_descriptor_storage_images;
        std::uint32_t                     max_per_stage_descriptor_input_attachments;
        std::uint32_t                     max_per_stage_resources;
        std::uint32_t                     max_descriptor_set_samplers;
        std::uint32_t                     max_descriptor_set_uniform_buffers;
        std::uint32_t                     max_descriptor_set_uniform_buffers_dynamic;
        std::uint32_t                     max_descriptor_set_storage_buffers;
        std::uint32_t                     max_descriptor_set_storage_buffers_dynamic;
        std::uint32_t                     max_descriptor_set_sampled_images;
        std::uint32_t                     max_descriptor_set_storage_images;
        std::uint32_t                     max_descriptor_set_input_attachments;
        std::uint32_t                     max_vertex_input_attributes;
        std::uint32_t                     max_vertex_input_bindings;
        std::uint32_t                     max_vertex_input_attribute_offset;
        std::uint32_t                     max_vertex_input_binding_stride;
        std::uint32_t                     max_vertex_output_components;
        std::uint32_t                     max_tessellation_generation_level;
        std::uint32_t                     max_tessellation_patch_size;
        std::uint32_t                     max_tessellation_control_per_vertex_input_components;
        std::uint32_t                     max_tessellation_control_per_vertex_output_components;
        std::uint32_t                     max_tessellation_control_per_patch_output_components;
        std::uint32_t                     max_tessellation_control_total_output_components;
        std::uint32_t                     max_tessellation_evaluation_input_components;
        std::uint32_t                     max_tessellation_evaluation_output_components;
        std::uint32_t                     max_geometry_shader_invocations;
        std::uint32_t                     max_geometry_input_components;
        std::uint32_t                     max_geometry_output_components;
        std::uint32_t                     max_geometry_output_vertices;
        std::uint32_t                     max_geometry_total_output_components;
        std::uint32_t                     max_fragment_input_components;
        std::uint32_t                     max_fragment_output_attachments;
        std::uint32_t                     max_fragment_dual_src_attachments;
        std::uint32_t                     max_fragment_combined_output_resources;
        std::uint32_t                     max_compute_shared_memory_size;
        std::array<std::uint32_t, 3>      max_compute_work_group_count;
        std::uint32_t                     max_compute_work_group_invocations;
        std::array<std::uint32_t, 3>      max_compute_work_group_size;
        std::uint32_t                     sub_pixel_precision_bits;
        std::uint32_t                     sub_texel_precision_bits;
        std::uint32_t                     mipmap_precision_bits;
        std::uint32_t                     max_draw_indexed_index_value;
        std::uint32_t                     max_draw_indirect_count;
        float                             max_sampler_lod_bias;
        float                             max_sampler_anisotropy;
        std::uint32_t                     max_viewports;
        std::array<std::uint32_t, 2>      max_viewport_dimensions;
        std::array<float, 2>              viewport_bounds_range;
        std::uint32_t                     viewport_sub_pixel_bits;
        std::int32_t                      min_texel_offset;
        std::uint32_t                     max_texel_offset;
        std::int32_t                      min_texel_gather_offset;
        std::uint32_t                     max_texel_gather_offset;
        float                             min_interpolation_offset;
        float                             max_interpolation_offset;
        std::uint32_t                     sub_pixel_interpolation_offset_bits;
        std::uint32_t                     max_framebuffer_width;
        std::uint32_t                     max_framebuffer_height;
        std::uint32_t                     max_framebuffer_layers;
        std::uint32_t                     framebuffer_color_sample_counts;
        std::uint32_t                     framebuffer_depth_sample_counts;
        std::uint32_t                     framebuffer_stencil_sample_counts;
        std::uint32_t                     framebuffer_no_attachments_sample_counts;
        std::uint32_t                     max_color_attachments;
        std::uint32_t                     sampled_image_color_sample_counts;
        std::uint32_t                     sampled_image_integer_sample_counts;
        std::uint32_t                     sampled_image_depth_sample_counts;
        std::uint32_t                     sampled_image_stencil_sample_counts;
        std::uint32_t                     storage_image_sample_counts;
        std::uint32_t                     max_sample_mask_words;
        float                             timestamp_period;
        std::uint32_t                     max_clip_distances;
        std::uint32_t                     max_cull_distances;
        std::uint32_t                     max_combined_clip_and_cull_distances;
        std::uint32_t                     discrete_queue_priorities;
        std::array<float, 2>              point_size_range;
        std::array<float, 2>              line_width_range;
        float                             point_size_granularity;
        float                             line_width_granularity;
        bool                              timestamp_compute_and_graphics;
        bool                              strict_lines;
        bool                              standard_sample_locations;
#ifdef _MSC_VER
    #pragma warning(suppress : 4820)
#endif
    };
#ifdef _MSC_VER
#pragma warning(default : 4820)
#endif
}
