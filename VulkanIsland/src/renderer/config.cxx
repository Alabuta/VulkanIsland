#include "config.hxx"


namespace renderer
{
    config adjust_renderer_config(vulkan::device_limits const &device_limits)
    {
        auto sample_counts = std::min(device_limits.framebuffer_color_sample_counts, device_limits.framebuffer_depth_sample_counts);

        renderer::config renderer_config;

        renderer_config.framebuffer_sample_counts = std::min(sample_counts, renderer_config.framebuffer_sample_counts);

        return renderer_config;
    }
}
