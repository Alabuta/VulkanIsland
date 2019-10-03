#pragma once

#include <memory>
#include <unordered_map>

#include <boost/functional/hash_fwd.hpp>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "device/device.hxx"
#include "resources/resource.hxx"
#include "renderer/graphics.hxx"

#if 0
namespace shader
{
    enum class STAGE {
        VERTEX = 0x01,
        TESS_CONTROL = 0x02,
        TESS_EVAL = 0x04,
        GEOMETRY = 0x08,
        FRAGMENT = 0x10,
        COMPUTE = 0x20,

        ALL_GRAPHICS_SHADER_STAGES = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT,
        ALL = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT | COMPUTE
    };

    template<STAGE S>
    struct stage {
        static auto constexpr index{static_cast<std::uint32_t>(S)};

        template<STAGE st>
        auto constexpr operator< (stage<st>) const noexcept { return S < st; }

        template<STAGE st>
        auto constexpr operator== (stage<st>) const noexcept { return S == st; }
    };

}

namespace shader
{
    struct vertex : stage<STAGE::VERTEX> { };
    struct tessctrl : stage<STAGE::TESS_CONTROL> { };
    struct tesseval : stage<STAGE::TESS_EVAL> { };
    struct geometry : stage<STAGE::GEOMETRY> { };
    struct fragment : stage<STAGE::FRAGMENT> { };
    struct compute : stage<STAGE::COMPUTE> { };

    using stages = std::variant<
        vertex,
        tessctrl,
        tesseval,
        geometry,
        fragment,
        compute
    >;
}

class VulkanShaderModule final {
public:

    VulkanShaderModule(VkShaderModule handle) noexcept : handle_{handle} { }

    VkShaderModule handle() const noexcept { return handle_; }

private:

    VkShaderModule handle_;

    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanShaderModule const &) = delete;
    VulkanShaderModule(VulkanShaderModule &&) = delete;
};

struct ShaderStage final {
    graphics::SHADER_STAGE semantic;

    std::string moduleName;
    std::string entryPoint;

    std::vector<std::int32_t> constants{};

    struct hash_value final {
        template<class T> requires std::same_as<ShaderStage, std::remove_cvref_t<T>>
        auto constexpr operator() (T &&shaderStage) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, shaderStage.semantic);

            boost::hash_combine(seed, shaderStage.moduleName);
            boost::hash_combine(seed, shaderStage.entryPoint);

            for (auto constant : shaderStage.constants)
                boost::hash_combine(seed, constant);

            // boost::hash_range(seed, shaderStage.constants);

            return seed;
        }
    };

    struct equal_comparator final {
        template<class T1, class T2> requires mpl::all_same<struct ShaderStage, T1, T2>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            auto constansAreEqual = std::size(lhs.constants) == std::size(rhs.constants) &&
                std::equal(std::cbegin(lhs.constants), std::cend(lhs.constants), std::cbegin(rhs.constants));

            return lhs.semantic == rhs.semantic &&lhs.moduleName == rhs.moduleName &&lhs.entryPoint == rhs.entryPoint &&constansAreEqual;
        }
    };
};


namespace program
{
    struct specialization_constant final {
        std::uint32_t id;
        std::variant<std::uint32_t, float> value;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator== (T &&constant) const
        {
            return value == constant.value && id == constant.id;
        }

        template<class T> requires std::same_as<std::remove_cvref_t<T>, specialization_constant>
        auto constexpr operator< (T &&constant) const
        {
            return id < constant.id;
        }
    };

    struct shader_stage final {
        shader::STAGE semantic;

        std::string module_name;
        std::uint32_t techique_index;

        std::set<program::specialization_constant> constants;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, shader_stage>
        auto constexpr operator== (T &&stage) const
        {
            return semantic == stage.semantic &&
                module_name == stage.module_name &&
                techique_index == stage.techique_index &&
                constants == stage.constants;
        }

        struct hash final {
        template<class T> requires std::same_as<std::remove_cvref_t<T>, shader_stage>
            auto constexpr operator() (T &&stage) const noexcept
            {
                std::size_t seed = 0;

                boost::hash_combine(seed, stage.semantic);
                boost::hash_combine(seed, stage.module_name);
                boost::hash_combine(seed, stage.techique_index);

                for (auto [id, value] : stage.constants) {
                    boost::hash_combine(seed, id);
                    boost::hash_combine(seed, value);
                }

                return seed;
            }
        };
    };
}


class ShaderManager final {
public:

    ShaderManager(VulkanDevice &vulkanDevice) noexcept : vulkan_device_{vulkanDevice} { }

    void CreateShaderPrograms(class Material const *const material);

    [[nodiscard]] VkPipelineShaderStageCreateInfo const &shaderStageProgram(ShaderStage const &shaderStage) const;

    [[nodiscard]] std::shared_ptr<VulkanShaderModule> shader_module(std::string_view name, std::uint32_t technique_index);

    [[nodiscard]] VkPipelineShaderStageCreateInfo pipeline_shader_stage(program::shader_stage const &shader_stage);

public:

    VulkanDevice &vulkan_device_;

    std::unordered_map<std::string, std::shared_ptr<VulkanShaderModule>> shaderModules_;

    std::unordered_map<ShaderStage, VkPipelineShaderStageCreateInfo, ShaderStage::hash_value, ShaderStage::equal_comparator> shaderStagePrograms_;
    std::unordered_map<ShaderStage, std::vector<VkSpecializationMapEntry>, ShaderStage::hash_value, ShaderStage::equal_comparator> specializationMapEntries_;
    std::unordered_map<ShaderStage, VkSpecializationInfo, ShaderStage::hash_value, ShaderStage::equal_comparator> specializationInfos_;

    /*std::unordered_map<shader_stage, VkPipelineShaderStageCreateInfo, shader_stage::hash_value> shaderStagePrograms2_;
    std::unordered_map<shader_stage, std::vector<VkSpecializationMapEntry>, shader_stage::hash_value> specializationMapEntries2_;
    std::unordered_map<shader_stage, VkSpecializationInfo, shader_stage::hash_value> specializationInfos2_;*/

    [[nodiscard]] std::shared_ptr<VulkanShaderModule> create_shader_module(std::vector<std::byte> const &shader_byte_code);

    std::map<std::pair<std::string, std::uint32_t>, std::shared_ptr<VulkanShaderModule>> modules_by_techinques_;
    std::unordered_map<program::shader_stage, VkPipelineShaderStageCreateInfo, program::shader_stage::hash> pipeline_shader_stages_;
};
#endif
