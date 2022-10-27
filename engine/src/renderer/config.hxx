#pragma once

#include <cstdint>

#include "vulkan/device_limits.hxx"


namespace render
{
    static std::uint32_t constexpr kCONCURRENTLY_PROCESSED_FRAMES{2};

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4820)
#endif
    struct config final {
        bool reversed_depth{true};

        bool generate_mipmaps{true};
        bool anisotropy_enabled{true};
        float max_anisotropy_level{16.f};

        std::uint32_t framebuffer_sample_counts{0x10};
    };
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

    render::config adjust_renderer_config(vulkan::device_limits const &device_limits);
}
