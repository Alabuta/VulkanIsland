#include <iostream>
#include <unordered_set>

#include <boost/functional/hash_fwd.hpp>

#include "utility/helpers.hxx"
#include "input_data.hxx"
#include "input_manager.hxx"


namespace platform
{
    void input_manager::update(platform::raw &data)
    {
        std::visit(overloaded{
            [this] (platform::mouse_data::raw &data)
            {
                mouse_.update(data);
            },
            [] (auto &&) { }
        }, data);
    }
}
