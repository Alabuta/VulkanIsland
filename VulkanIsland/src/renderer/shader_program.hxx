#pragma once

#include <unordered_map>
#include <concepts>
#include <variant>
#include <memory>
#include <vector>
#include <set>

#include <boost/cstdfloat.hpp>

#include "device/device.hxx"
#include "graphics.hxx"


namespace graphics
{
    struct specialization_constant final {
        std::uint32_t id;
        std::variant<std::uint32_t, boost::float32_t> value;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator== (T && constant) const
        {
            return value == constant.value && id == constant.id;
        }

        template<class T> requires std::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator< (T && constant) const
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
        std::uint32_t techique_index;

        graphics::SHADER_STAGE semantic;

        std::set<graphics::specialization_constant> constants;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, shader_stage>
        auto constexpr operator== (T &&stage) const
        {
            return module_name == stage.module_name &&
                techique_index == stage.techique_index &&
                semantic == stage.semantic &&
                constants == stage.constants;
        }
    };

    struct shader_program final {
        std::vector<graphics::shader_stage> stages;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, shader_program>
        auto constexpr operator== (T &&program) const
        {
            return stages == program.stages;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::specialization_constant> {
        std::size_t operator() (graphics::specialization_constant const &specialization_constant) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, specialization_constant.id);
            boost::hash_combine(seed, specialization_constant.value);

            return seed;
        }
    };

    template<>
    struct hash<graphics::shader_stage> {
        std::size_t operator() (graphics::shader_stage const &stage) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, stage.module_name);
            boost::hash_combine(seed, stage.techique_index);
            boost::hash_combine(seed, stage.semantic);

            for (auto [id, value] : stage.constants) {
                boost::hash_combine(seed, id);
                boost::hash_combine(seed, value);
            }

            return seed;
        }
    };

    template<>
    struct hash<graphics::shader_program> {
        std::size_t operator() (graphics::shader_program const &program) const
        {
            std::size_t seed = 0;

            graphics::hash<graphics::shader_stage> constexpr shader_stage_hasher;

            for (auto &&stage : program.stages)
                boost::hash_combine(seed, shader_stage_hasher(stage));

            return seed;
        }
    };
}

namespace graphics
{
    class shader_manager final {
    public:

        shader_manager(VulkanDevice &vulkan_device) noexcept : vulkan_device_{vulkan_device} { }

    private:

        VulkanDevice &vulkan_device_;

        // GAPI
        std::map<std::pair<std::string, std::uint32_t>, std::shared_ptr<shader_module>> modules_by_techinques_;

        using sc_map_entry = VkSpecializationMapEntry;
        using sc_info = VkSpecializationInfo;

        std::unordered_map<shader_stage, std::vector<sc_map_entry>, hash<shader_stage>> specialization_map_entries_;
        std::unordered_map<shader_stage, sc_info, hash<shader_stage>> specializations_;

        [[nodiscard]] std::shared_ptr<graphics::shader_module> create_shader_module(std::string_view name, std::uint32_t technique_index);
    };
}
