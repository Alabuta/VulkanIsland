#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "attachments.hxx"



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


class MaterialFactory final {
public:

    [[nodiscard]] std::shared_ptr<Material> CreateMaterial() noexcept;

private:

    struct MaterialProperties final {
        VkPipelineRasterizationStateCreateInfo rasterizationState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendState;
    };

    std::unordered_map<std::shared_ptr<Material>, MaterialProperties> materialProperties_;
};
