#pragma once

#include <vector>
#include <variant>
#include <set>

#include <boost/cstdfloat.hpp>

#include "graphics.hxx"


namespace graphics
{
struct specialization_constant final {
    std::uint32_t id;
    std::variant<std::uint32_t, boost::float32_t> value;

    template<class T, typename std::enable_if_t<std::is_same_v<specialization_constant, std::decay_t<T>>>* = nullptr>
    auto constexpr operator== (T &&constant) const
    {
        return value == constant.value && id == constant.id;
    }

    template<class T, typename std::enable_if_t<std::is_same_v<specialization_constant, std::decay_t<T>>>* = nullptr>
    auto constexpr operator< (T &&constant) const
    {
        return id < constant.id;
    }
};

struct shader_stage final {
    std::string module_name;
    std::uint32_t techique_index;

    graphics::PIPELINE_SHADER_STAGE semantic;

    std::set<graphics::specialization_constant> constants;

    template<class T, typename std::enable_if_t<std::is_same_v<shader_stage, std::decay_t<T>>>* = nullptr>
    auto constexpr operator== (T &&stage) const
    {
        return module_name == stage.module_name &&
            techique_index == stage.techique_index &&
            semantic == stage.semantic &&
            constants == stage.constants;
    }
};
}

namespace graphics
{
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
}
