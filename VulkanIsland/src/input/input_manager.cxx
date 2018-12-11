#include <iostream>

#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/functional/hash_fwd.hpp>

#include "input_manager.hxx"
#include "../input/input_data_types.hxx"


using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}


void InputManager::onUpdate(input::RawData &data)
{
    std::visit(overloaded{
        [this] (input::mouse::RawData &coords)
        {
            ;
        },
        [] (auto &&) { }
    }, data);
}
