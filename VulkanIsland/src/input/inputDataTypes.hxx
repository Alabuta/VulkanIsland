#pragma once

#include <bitset>
#include <variant>


namespace input
{
    namespace mouse
    {
        struct buttons final {
            std::bitset<16> value;
        };

        struct relative_coords final {
            float x, y;
        };

        struct wheel_data final {
            float xoffset, yoffset;
        };

        using raw_data = std::variant<
            buttons, relative_coords, wheel_data
        >;
    }

    namespace keyboard
    {
        using raw_data = bool;
    }

    using raw_data = std::variant<mouse::raw_data, keyboard::raw_data>;
}
