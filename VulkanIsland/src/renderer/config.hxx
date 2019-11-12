#pragma once

#include <cstdint>


namespace renderer
{
    struct config final {
        bool reversed_depth{true};

        bool anisotropy_enabled{true};
        float max_anisotropy_level{16.f};

        std::uint32_t framebuffer_sample_counts{0x10};
    };
}
