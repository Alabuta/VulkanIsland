#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
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

    ;

private:

};

class NormalsDebugMaterial final : public Material {
public:

    ;

private:

};


class MaterialFactory final {
public:

    template<class T>
    [[nodiscard]] std::shared_ptr<T> CreateMaterial() noexcept;


    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> material);// const noexcept;

private:

    std::unordered_map<std::shared_ptr<Material>, MaterialProperties> materialProperties_;

    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;


    std::unordered_map<std::string, std::shared_ptr<VulkanShaderModule>> shaderModules_;
};

template<class T>
std::shared_ptr<T> MaterialFactory::CreateMaterial() noexcept
{
    auto material = std::make_shared<T>();

    /*if (materialProperties_.count(material) == 0)
        return { };*/

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

    return material;
}
