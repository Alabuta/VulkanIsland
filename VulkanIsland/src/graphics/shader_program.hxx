#pragma once

#include <unordered_map>
#include <variant>
#include <memory>
#include <vector>
#include <map>
#include <set>

#include <boost/cstdfloat.hpp>

#include "utility/mpl.hxx"
#include "vulkan/device.hxx"
#include "graphics.hxx"


namespace graphics
{
    struct specialization_constant final {
        std::uint32_t id;
        std::variant<std::int32_t, boost::float32_t> value;

        template<class T> requires mpl::variant_alternative<std::remove_cvref_t<T>, decltype(value)>
        specialization_constant(std::uint32_t id, T value) : id{id}, value{value} { }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator== (T &&constant) const
        {
            return value == constant.value && id == constant.id;
        }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator< (T &&constant) const
        {
            return id < constant.id;
        }
    };
}

namespace graphics
{
    class shader_module final {
    public:

        shader_module(VkShaderModule handle) noexcept : handle_{handle} { }

        VkShaderModule handle() const noexcept { return handle_; }

    private:

        VkShaderModule handle_;

        shader_module() = delete;
        shader_module(shader_module const &) = delete;
        shader_module(shader_module &&) = delete;
    };
}

namespace graphics
{
    struct shader_stage final {
        std::string module_name;
        std::uint32_t technique_index;

        graphics::SHADER_STAGE semantic;

        std::set<graphics::specialization_constant> constants;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, shader_stage>
        auto constexpr operator== (T &&stage) const
        {
            return module_name == stage.module_name &&
                technique_index == stage.technique_index &&
                semantic == stage.semantic &&
                constants == stage.constants;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::specialization_constant> {
        std::size_t operator() (graphics::specialization_constant const &specialization_constant) const;
    };

    template<>
    struct hash<graphics::shader_stage> {
        std::size_t operator() (graphics::shader_stage const &stage) const;
    };
}

namespace graphics
{
    class shader_manager final {
    public:

        shader_manager(vulkan::device &device) noexcept : device_{device} { }

        [[nodiscard]] std::shared_ptr<graphics::shader_module> shader_module(std::string_view name);

    private:

        vulkan::device &device_;

        std::map<std::string, std::shared_ptr<graphics::shader_module>> shader_modules_;

        [[nodiscard]] std::shared_ptr<graphics::shader_module> create_shader_module(std::string_view name);
    };
}
