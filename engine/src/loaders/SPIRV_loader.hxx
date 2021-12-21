#pragma once

#ifdef max
#undef max
#endif

#include <vector>
#include <string_view>


namespace loader
{
    std::vector<std::byte> load_SPIRV(std::string_view name);
}
