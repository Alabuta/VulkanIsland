#pragma once

#include <memory>
#include <unordered_map>

#include <boost/functional/hash_fwd.hpp>

#include "main.hxx"
#include "device/device.hxx"
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

    std::vector<std::int32_t> constants{};

    struct hash_value final {
        template<class T, typename std::enable_if_t<std::is_same_v<ShaderStage, std::decay_t<T>>>...>
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
        template<class T1, class T2, typename std::enable_if_t<are_same_v<struct ShaderStage, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            auto constansAreEqual = std::size(lhs.constants) == std::size(rhs.constants) &&
                std::equal(std::cbegin(lhs.constants), std::cend(lhs.constants), std::cbegin(rhs.constants));

            return lhs.semantic == rhs.semantic && lhs.moduleName == rhs.moduleName && lhs.entryPoint == rhs.entryPoint && constansAreEqual;
        }
    };
};



class ShaderManager final {
public:

    ShaderManager(VulkanDevice &vulkanDevice) noexcept : vulkanDevice_{vulkanDevice} { }

    void CreateShaderPrograms(class Material const *const material);

    [[nodiscard]] VkPipelineShaderStageCreateInfo const &shaderStageProgram(ShaderStage const &shaderStage) const;

public:

    VulkanDevice &vulkanDevice_;

    std::unordered_map<std::string, std::shared_ptr<VulkanShaderModule>> shaderModules_;
    std::unordered_map<ShaderStage, VkPipelineShaderStageCreateInfo, ShaderStage::hash_value, ShaderStage::equal_comparator> shaderStagePrograms_;

    std::unordered_map<ShaderStage, std::vector<VkSpecializationMapEntry>, ShaderStage::hash_value, ShaderStage::equal_comparator> specializationMapEntries_;
    std::unordered_map<ShaderStage, VkSpecializationInfo, ShaderStage::hash_value, ShaderStage::equal_comparator> specializationInfos_;

    [[nodiscard]] std::shared_ptr<VulkanShaderModule> CreateShaderModule(std::vector<std::byte> const &shaderByteCode);
};
