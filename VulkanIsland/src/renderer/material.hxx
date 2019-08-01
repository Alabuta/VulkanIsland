#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "device/device.hxx"
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

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
};



class Material {
public:

    virtual ~Material() = default;

    [[nodiscard]] virtual std::vector<ShaderStage> const &shaderStages() const = 0;

    RasterizationState rasterizationState;
    DepthStencilState depthStencilState;
    ColorBlendState colorBlendState;


private:

    // resources
    //// shader handle
    //// rasterization state
    //// depth and stencil state
    //// blending
    // pipeline layout (descriptor set layout)
};

class Material2 final {
public:

    std::vector<ShaderStage2> shaderStages;

    RasterizationState rasterizationState;
    DepthStencilState depthStencilState;
    ColorBlendState colorBlendState;


private:
};


class MaterialFactory final {
public:

    MaterialFactory(ShaderManager &shaderManager) noexcept : shaderManager_{shaderManager} { }

    [[nodiscard]] std::shared_ptr<Material> CreateMaterial(std::string_view type);
    [[nodiscard]] std::shared_ptr<Material2> CreateMaterial2(std::string_view name, std::uint32_t technique = 0);

    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> const material);// const noexcept;

    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> const &pipelineShaderStages(std::shared_ptr<Material> const material);

    std::map<std::string, std::shared_ptr<Material>> const materials() const noexcept { return materials_; }


private:

    ShaderManager &shaderManager_;

    std::map<std::string, std::shared_ptr<Material>> materials_;
    std::map<std::shared_ptr<Material>, MaterialProperties, std::owner_less<std::shared_ptr<Material>>> materialProperties_;

    std::map<std::shared_ptr<Material>, std::vector<VkPipelineShaderStageCreateInfo>, std::owner_less<std::shared_ptr<Material>>> pipelineShaderStages_;

    std::map<std::string, std::shared_ptr<Material2>> materials2_;
    std::map<std::shared_ptr<Material2>, MaterialProperties, std::owner_less<std::shared_ptr<Material2>>> materialProperties2_;

    std::map<std::shared_ptr<Material2>, std::vector<VkPipelineShaderStageCreateInfo>, std::owner_less<std::shared_ptr<Material2>>> pipelineShaderStages2_;


    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;
    void InitMaterialProperties(std::shared_ptr<Material2> material);
};
