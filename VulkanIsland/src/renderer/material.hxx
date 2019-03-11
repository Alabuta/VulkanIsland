#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "device.hxx"
#include "resources/resource.hxx"

#include "attachments.hxx"
#include "resources/program.hxx"



enum class CULL_MODE {
    NONE, FRONT, BACK, FRONT_AND_BACK = FRONT | BACK
};

enum class POLYGON_FRONT_FACE {
    COUNTER_CLOCKWISE, CLOCKWISE
};

enum class POLYGON_MODE {
    FILL, LINE, POINT
};


struct RasterizationState final {
    CULL_MODE cullMode{CULL_MODE::BACK};
    POLYGON_FRONT_FACE frontFace{POLYGON_FRONT_FACE::COUNTER_CLOCKWISE};
    POLYGON_MODE polygonMode{POLYGON_MODE::FILL};

    float lineWidth{1.f};
};


enum class COMPARE_OPERATION {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS
};

struct DepthStencilState final {
    bool depthTestEnable{true};
    bool depthWriteEnable{true};

    COMPARE_OPERATION depthCompareOperation{COMPARE_OPERATION::GREATER};

    bool stencilTestEnable{false};
};


struct MaterialProperties final {
    VkPipelineRasterizationStateCreateInfo rasterizationState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
};



class Material {
public:

    static auto constexpr kVERTEX_FILE_NAME{""sv};
    static auto constexpr kFRAGMENT_FILE_NAME{""sv};

    RasterizationState rasterizationState;
    DepthStencilState depthStencilState;
    ColorBlendState colorBlendState;


private:

    // resources
    // shader handle
    //// rasterization state
    //// depth and stencil state
    //// blending
    // pipeline layout (descriptor set layout)
};

class TexCoordsDebugMaterial final : public Material {
public:

    static auto constexpr kVERTEX_FILE_NAME{R"(test/vertA.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(test/fragA.spv)"sv};


private:

};

class NormalsDebugMaterial final : public Material {
public:

    static auto constexpr kVERTEX_FILE_NAME{R"(test/vertB.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(test/fragB.spv)"sv};


private:

};

class TestMaterial final : public Material {
public:

    static auto constexpr kVERTEX_FILE_NAME{R"(vert.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(frag.spv)"sv};


private:

};


class MaterialFactory final {
public:

    MaterialFactory(VulkanDevice &vulkanDevice) noexcept : vulkanDevice_{vulkanDevice} { }

    template<class T>
    [[nodiscard]] std::shared_ptr<T> CreateMaterial(ShaderManager &shaderManager);


    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> material);// const noexcept;

    template<class T>
    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> const &pipelineShaderStages() const;

private:

    VulkanDevice &vulkanDevice_;

    std::map<std::shared_ptr<Material>, MaterialProperties, std::owner_less<std::shared_ptr<Material>>> materialProperties_;

    std::unordered_map<std::string, std::vector<VkPipelineShaderStageCreateInfo>> shaderStages_;
    std::unordered_map<std::string_view, std::shared_ptr<VulkanShaderModule>> shaderModules_;


    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;

    template<class T>
    void InitShaderStages(ShaderManager &shaderManager);
};

template<class T>
std::shared_ptr<T> MaterialFactory::CreateMaterial(ShaderManager &shaderManager)
{
    static_assert(std::is_base_of_v<Material, T>, "material type has to be derived from base 'Material' type");
    
    auto material = std::make_shared<T>();

    material->colorBlendState.attachments.push_back(ColorBlendAttachmentState{
        false,
        BLEND_FACTOR::ONE,
        BLEND_FACTOR::ZERO,
        BLEND_OPERATION::ADD,
        BLEND_FACTOR::ONE,
        BLEND_FACTOR::ZERO,
        BLEND_OPERATION::ADD,
        COLOR_COMPONENT::RGBA
    });

    InitMaterialProperties(material);

    InitShaderStages<T>(shaderManager);

    return material;
}

template<class T>
void MaterialFactory::InitShaderStages(ShaderManager &shaderManager)
{
    static_assert(std::is_base_of_v<Material, T>, "material type has to be derived from base 'Material' type");

    auto names = std::string{T::kVERTEX_FILE_NAME} + std::string{T::kFRAGMENT_FILE_NAME};

    if (shaderStages_.count(names) == 0) {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        {
            std::shared_ptr<VulkanShaderModule> shaderModule;

            if (shaderModules_.count(T::kVERTEX_FILE_NAME) == 0) {
                auto const shaderByteCode = shaderManager.ReadShaderFile(T::kVERTEX_FILE_NAME);

                if (shaderByteCode.empty())
                    throw std::runtime_error("failed to open vertex shader file"s);

                shaderModule = shaderManager.CreateShaderModule(shader::STAGE::VERTEX, shaderByteCode);

                shaderModules_.emplace(T::kVERTEX_FILE_NAME, shaderModule);
            }

            else shaderModule = shaderModules_.at(T::kVERTEX_FILE_NAME);

            shaderStages.push_back(VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr, 0,
                VK_SHADER_STAGE_VERTEX_BIT,
                shaderModule->handle(),
                "main",
                nullptr
            });
        }

        {
            std::shared_ptr<VulkanShaderModule> shaderModule;

            if (shaderModules_.count(T::kFRAGMENT_FILE_NAME) == 0) {
                auto const shaderByteCode = shaderManager.ReadShaderFile(T::kFRAGMENT_FILE_NAME);

                if (shaderByteCode.empty())
                    throw std::runtime_error("failed to open fragment shader file"s);

                shaderModule = shaderManager.CreateShaderModule(shader::STAGE::FRAGMENT, shaderByteCode);

                shaderModules_.emplace(T::kFRAGMENT_FILE_NAME, shaderModule);
            }

            else shaderModule = shaderModules_.at(T::kFRAGMENT_FILE_NAME);

            shaderStages.push_back(VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr, 0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                shaderModule->handle(),
                "main",
                nullptr
            });

            shaderModules_.emplace(T::kFRAGMENT_FILE_NAME, shaderModule);
        }

        shaderStages_.emplace(names, std::move(shaderStages));
    }
}

template<class T>
std::vector<VkPipelineShaderStageCreateInfo> const &MaterialFactory::pipelineShaderStages() const
{
    auto names = std::string{T::kVERTEX_FILE_NAME} +std::string{T::kFRAGMENT_FILE_NAME};

    if (shaderStages_.count(names) == 0)
        throw std::runtime_error("failed to find shader modules"s);

    return shaderStages_.at(names);
}
