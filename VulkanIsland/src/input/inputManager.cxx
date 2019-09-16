#include <iostream>

#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/functional/hash_fwd.hpp>

#include "utility/helpers.hxx"
#include "inputDataTypes.hxx"
#include "inputManager.hxx"


using namespace std::string_literals;
using namespace std::string_view_literals;


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
