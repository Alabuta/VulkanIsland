#pragma once
#include <array>
#include <vector>


enum class CULL_MODE {
    NONE, FRONT, BACK, FRONT_AND_BACK
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
};


enum class DEPTH_COMPARE_OPERATION {
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

    DEPTH_COMPARE_OPERATION depthCompareOperation{DEPTH_COMPARE_OPERATION::GREATER};

    bool stencilTestEnable{false};
};


enum class BLEND_STATE_OPERATION {
    CLEAR,
    AND,
    AND_REVERSE,
    COPY,
    AND_INVERTED,
    NO_OP,
    XOR,
    OR,
    NOR,
    EQUIVALENT,
    INVERT,
    OR_REVERSE,
    COPY_INVERTED,
    OR_INVERTED,
    NAND,
    SET
};

enum class BLEND_FACTOR {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
    SRC1_COLOR,
    ONE_MINUS_SRC1_COLOR,
    SRC1_ALPHA,
    ONE_MINUS_SRC1_ALPHA,
};

enum class BLEND_OPERATION {
    ADD,
    SUBTRACT
};

enum class COLOR_COMPONENT {
    R = 0x01,
    G = 0x02,
    B = 0x04,
    A = 0x08,

    RGB = R | G | B,
    RGBA = R| G| B| A
};

struct ColorBlendAttachmentState final {
    bool blendEnable{false};

    BLEND_FACTOR srcColorBlendFactor{BLEND_FACTOR::ONE};
    BLEND_FACTOR dstColorBlendFactor{BLEND_FACTOR::ZERO};

    BLEND_OPERATION colorBlendOperation{BLEND_OPERATION::ADD};

    BLEND_FACTOR srcAlphaBlendFactor{BLEND_FACTOR::ONE};
    BLEND_FACTOR dstAlphaBlendFactor{BLEND_FACTOR::ZERO};

    BLEND_OPERATION alphaBlendOperation{BLEND_OPERATION::ADD};

    COLOR_COMPONENT colorWriteMask{COLOR_COMPONENT::RGBA};
};


struct ColorBlendState final {
    bool logicOperationEnable{false};

    BLEND_STATE_OPERATION logicOperation{BLEND_STATE_OPERATION::COPY};

    std::array<float, 4> blendConstants{0, 0, 0, 0};

    std::vector<ColorBlendAttachmentState> attachments;
};


class Material {
public:

    ;

private:

    RasterizationState rasterizationState;
    DepthStencilState depthStencilState;
    ColorBlendState colorBlendState;

    // resources
    // shader handle
    //// rasterization state
    //// depth and stencil state
    //// blending
    // pipeline layout (descriptor set layout)
};
