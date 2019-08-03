#pragma once

#include <memory>
#include <string_view>
using namespace std::string_view_literals;

#include "renderer/material.hxx"
#include "renderer/material_description.hxx"


namespace loader
{
[[nodiscard]] std::shared_ptr<material_description> load_material_description(std::string_view name);
}
