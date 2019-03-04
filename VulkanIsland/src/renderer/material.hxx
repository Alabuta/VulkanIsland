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


class MaterialFactory final {
public:

    MaterialFactory(VulkanDevice &vulkanDevice) noexcept : vulkanDevice_{vulkanDevice} { }

    template<class T>
    [[nodiscard]] std::shared_ptr<T> CreateMaterial();


    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> material);// const noexcept;

    template<class T>
    [[nodiscard]] std::optional<VkPipelineShaderStageCreateInfo> MaterialFactory::pipelineShaderStage() const

private:

    VulkanDevice &vulkanDevice_;

    std::unordered_map<std::shared_ptr<Material>, MaterialProperties> materialProperties_;

    std::unordered_map<std::string_view, VkPipelineShaderStageCreateInfo> shaderStages_;


    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;

    template<class T>
    void InitShaderStages();
};

template<class T>
std::shared_ptr<T> MaterialFactory::CreateMaterial()
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

    InitShaderStages<T>();

    return material;
}

template<class T>
void MaterialFactory::InitShaderStages()
{
    static_assert(std::is_base_of_v<Material, T>, "material type has to be derived from base 'Material' type");

    if (shaderStages_.count(T::kVERTEX_FILE_NAME) == 0) {
        auto const shaderByteCode = ReadShaderFile(T::kVERTEX_FILE_NAME);

        if (shaderByteCode.empty()) {
            std::cerr << "failed to open vertex shader file\n"s;
            return;
        }

        auto const shaderModule = vulkanDevice_.resourceManager().CreateShaderModule(shaderByteCode);

        VkPipelineShaderStageCreateInfo const shaderCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr, 0,
            VK_SHADER_STAGE_VERTEX_BIT,
            shaderModule->handle(),
            "main",
            nullptr
        };

        shaderStages_.emplace(T::kVERTEX_FILE_NAME, shaderCreateInfo);
    }

    if (shaderStages_.count(T::kFRAGMENT_FILE_NAME) == 0) {
        auto const shaderByteCode = ReadShaderFile(T::kFRAGMENT_FILE_NAME);

        if (shaderByteCode.empty()) {
            std::cerr << "failed to open fragment shader file\n"s;
            return;
        }

        auto const shaderModule = vulkanDevice_.resourceManager().CreateShaderModule(shaderByteCode);

        VkPipelineShaderStageCreateInfo const shaderCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr, 0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            shaderModule->handle(),
            "main",
            nullptr
        };

        shaderStages_.emplace(T::kFRAGMENT_FILE_NAME, shaderCreateInfo);
    }
}

template<class T>
std::optional<VkPipelineShaderStageCreateInfo> MaterialFactory::pipelineShaderStage() const
{
    if (shaderStages_.count(T::kVERTEX_FILE_NAME) == 0)
        return { };

    else shaderStages_.at(T::kVERTEX_FILE_NAME);
}
