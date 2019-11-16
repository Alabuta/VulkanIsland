#pragma once

#include <exception>
#include <string>


namespace memory
{
    struct exception : public std::exception {
        explicit exception(std::string const &what_arg) : std::exception(what_arg.c_str()) { }
    };

    struct bad_allocation final : public memory::exception {
        explicit bad_allocation(std::string const &what_arg) : memory::exception(what_arg) { }
    };

    struct logic_error final : public memory::exception {
        explicit logic_error(std::string const &what_arg) : memory::exception(what_arg) { }
    };
}

namespace resource
{
    struct exception : public std::exception {
        explicit exception(std::string const &what_arg) : std::exception(what_arg.c_str()) { }
    };

    struct instantiation_fail final : public resource::exception {
        explicit instantiation_fail(std::string const &what_arg) : resource::exception(what_arg) { }
    };

    struct memory_bind final : public resource::exception {
        explicit memory_bind(std::string const &what_arg) : resource::exception(what_arg) { }
    };
}
