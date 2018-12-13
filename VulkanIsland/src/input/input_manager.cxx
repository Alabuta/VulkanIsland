#include <iostream>

#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/functional/hash_fwd.hpp>

#include "input_data_types.hxx"
#include "input_manager.hxx"


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
        [this] (input::mouse::RawData &data)
        {
            mouse_.update(data);
        },
        [] (auto &&) { }
    }, data);
}
