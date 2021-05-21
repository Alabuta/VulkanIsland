#pragma once

#include <stdexcept>
#include <string>


namespace memory
{
    struct exception : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };

    struct bad_allocation final : public memory::exception {
        explicit bad_allocation(std::string const &what_arg) : memory::exception(what_arg) { }
    };
}

namespace resource
{
    struct exception : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };

    struct instantiation_fail final : public resource::exception {
        explicit instantiation_fail(std::string const &what_arg) : resource::exception(what_arg) { }
    };

    struct memory_bind final : public resource::exception {
        explicit memory_bind(std::string const &what_arg) : resource::exception(what_arg) { }
    };

    struct not_enough_memory final : public resource::exception {
        explicit not_enough_memory(std::string const &what_arg) : resource::exception(what_arg) { }
    };
}

namespace graphics
{
    struct exception final : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };
}

namespace loader
{
    struct exception final : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };
}

namespace vulkan
{
    struct exception : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };

    struct instance_exception final : public vulkan::exception {
        explicit instance_exception(std::string const &what_arg) : vulkan::exception(what_arg) { }
    };

    struct device_exception final : public vulkan::exception {
        explicit device_exception(std::string const &what_arg) : vulkan::exception(what_arg) { }
    };

    struct swapchain_exception final : public vulkan::exception {
        explicit swapchain_exception(std::string const &what_arg) : vulkan::exception(what_arg) { }
    };

    struct logic_error final : public vulkan::exception {
        explicit logic_error(std::string const &what_arg) : vulkan::exception(what_arg) { }
    };
}

namespace app
{
    struct exception final : public std::runtime_error {
        explicit exception(std::string const &what_arg) : std::runtime_error(what_arg.c_str()) { }
    };
}
