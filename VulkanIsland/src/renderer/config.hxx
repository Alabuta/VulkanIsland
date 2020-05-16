#pragma once

#include <cstdint>

#include "vulkan/device_limits.hxx"


namespace renderer
{
#pragma warning(push)
#pragma warning(disable : 4820)
    struct config final {
        bool reversed_depth{true};

        bool anisotropy_enabled{true};
        float max_anisotropy_level{16.f};

        std::uint32_t framebuffer_sample_counts{0x10};
    };
#pragma warning(pop)

    renderer::config adjust_renderer_config(vulkan::device_limits const &device_limits);
}
