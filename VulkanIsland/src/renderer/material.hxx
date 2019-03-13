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

    virtual ~Material() = default;

    static auto constexpr kVERTEX_FILE_NAME{""sv};
    static auto constexpr kFRAGMENT_FILE_NAME{""sv};

    virtual std::vector<ShaderStage> const &shaderStages() const noexcept = 0;

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

    ~TexCoordsDebugMaterial() override = default;

    static auto constexpr kVERTEX_FILE_NAME{R"(test/vertA.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(test/fragA.spv)"sv};

    std::vector<ShaderStage> const &shaderStages() const noexcept override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(test/vertA.spv)"s, "main"s},
            ShaderStage{shader::STAGE::FRAGMENT, R"(test/fragA.spv)"s, "main"s}
        };

        return shaderStages;
    }

private:

};

class NormalsDebugMaterial final : public Material {
public:

    ~NormalsDebugMaterial() override = default;

    static auto constexpr kVERTEX_FILE_NAME{R"(test/vertB.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(test/fragB.spv)"sv};

    std::vector<ShaderStage> const &shaderStages() const noexcept override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(test/vertB.spv)"s, "main"s},
            ShaderStage{shader::STAGE::FRAGMENT, R"(test/fragB.spv)"s, "main"s}
        };

        return shaderStages;
    }


private:

};

class TestMaterial final : public Material {
public:

    ~TestMaterial() override = default;

    static auto constexpr kVERTEX_FILE_NAME{R"(vert.spv)"sv};
    static auto constexpr kFRAGMENT_FILE_NAME{R"(frag.spv)"sv};

    std::vector<ShaderStage> const &shaderStages() const noexcept override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(vert.spv)"s, "main"s},
            ShaderStage{shader::STAGE::FRAGMENT, R"(frag.spv)"s, "main"s}
        };

        return shaderStages;
    }


private:

};


class MaterialFactory final {
public:

    MaterialFactory(ShaderManager &shaderManager) noexcept : shaderManager_{shaderManager} { }

    template<class T>
    [[nodiscard]] std::shared_ptr<T> CreateMaterial();

    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> const material);// const noexcept;

    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> const &pipelineShaderStages(std::shared_ptr<Material> const material);

private:

    ShaderManager &shaderManager_;

    std::map<std::shared_ptr<Material>, MaterialProperties, std::owner_less<std::shared_ptr<Material>>> materialProperties_;

    std::map<std::shared_ptr<Material>, std::vector<VkPipelineShaderStageCreateInfo>, std::owner_less<std::shared_ptr<Material>>> pipelineShaderStages_;


    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;
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

    shaderManager_.CreateShaderPrograms(material.get());

    return material;
}
