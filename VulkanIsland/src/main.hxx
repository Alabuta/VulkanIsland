#pragma once


#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <tuple>
#include <set>
#include <optional>
#include <string>
#include <string_view>

using namespace std::string_literals;
using namespace std::string_view_literals;

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "utility/helpers.hxx"

#define USE_WIN32 0

#include "config.hxx"





auto constexpr kREVERSED_DEPTH = true;
