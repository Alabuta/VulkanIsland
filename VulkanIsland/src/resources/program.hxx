#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "device.hxx"
#include "resources/resource.hxx"


namespace shader
{
    enum class STAGE {
        VERTEX = 0x01,
        TESS_CONTROL = 0x02,
        TESS_EVAL = 0x04,
        GEOMETRY = 0x08,
        FRAGMENT = 0x10,
        COMPUTE = 0x20
    };

    template<STAGE S>
    struct stage {
        template<STAGE s>
        auto constexpr operator< (stage<s>) const noexcept
        {
            return S < s;
        }
    };

    struct vertex : stage<STAGE::VERTEX> { };
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
    shader::STAGE semantic;

    std::string moduleName;
    std::string entryPoint;

    struct hash_value final {
        template<class T, typename std::enable_if_t<std::is_same_v<ShaderStage, std::decay_t<T>>>...>
        auto constexpr operator() (T &&shaderStage) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, shaderStage.semantic);

            boost::hash_combine(seed, shaderStage.moduleName);
            boost::hash_combine(seed, shaderStage.entryPoint);

            return seed;
        }
    };

    struct equal_comparator final {
        template<class T1, class T2, typename std::enable_if_t<are_same_v<struct ShaderStage, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            return lhs.semantic == rhs.semantic && lhs.moduleName == rhs.moduleName && lhs.entryPoint == rhs.entryPoint;
        }
    };

};



#if 0
namespace
{
struct ShaderSourceFilesNames final {
    std::string vertexFileName_;
    std::string fragmentFileName_;
};

template<class T>
ShaderSourceFilesNames constexpr ShadersNames() noexcept
{
    if constexpr (std::is_same_v<T, TexCoordsDebugMaterial>)
    {
        return {
            R"(test/vertA.spv)"s,
            R"(test/fragA.spv)"s
        };
    }

    else if constexpr (std::is_same_v<T, NormalsDebugMaterial>)
    {
        return {
            R"(test/vertB.spv)"s,
            R"(test/fragB.spv)"s
        };
    }

    else static_assert("material type has to be derived from common 'Material' type class");
}
}
#endif



class ShaderManager final {
public:

    ShaderManager(VulkanDevice &vulkanDevice) noexcept : vulkanDevice_{vulkanDevice} { }

    void CreateShaderPrograms(class Material const *const material);

    [[nodiscard]] VkPipelineShaderStageCreateInfo const &GetPipelineShaderStage(ShaderStage const &shaderStage) const;

public:

    VulkanDevice &vulkanDevice_;

    std::unordered_map<std::string, std::shared_ptr<VulkanShaderModule>> shaderModules_;
    std::unordered_map<ShaderStage, VkPipelineShaderStageCreateInfo, ShaderStage::hash_value, ShaderStage::equal_comparator> pipelineShaderStages_;

    [[nodiscard]] std::shared_ptr<VulkanShaderModule> CreateShaderModule(std::vector<std::byte> const &shaderByteCode);
};
