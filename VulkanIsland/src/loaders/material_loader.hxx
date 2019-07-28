#pragma once

#include <string_view>
using namespace std::string_view_literals;

#include "renderer/material.hxx"


namespace loader
{
[[nodiscard]] std::shared_ptr<Material2> load_material(std::string_view name);
}
