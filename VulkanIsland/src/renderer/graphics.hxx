#pragma once

#include <boost/functional/hash_fwd.hpp>


namespace graphics
{
    template<class T>
    struct hash;

    enum class PRIMITIVE_TOPOLOGY {
        POINTS = 0,
        LINES, LINE_STRIP,
        TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN
    };

    enum class PIPELINE_SHADER_STAGE {
        VERTEX = 0x01,
        TESS_CONTROL = 0x02,
        TESS_EVAL = 0x04,
        GEOMETRY = 0x08,
        FRAGMENT = 0x10,
        COMPUTE = 0x20,

        ALL_GRAPHICS = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT,
        ALL = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT | COMPUTE
    };

    enum class DESCRIPTOR_TYPE {
        SAMPLER = 0,
        COMBINED_IMAGE_SAMPLER,
        SAMPLED_IMAGE,
        STORAGE_IMAGE,
        UNIFORM_TEXEL_BUFFER,
        STORAGE_TEXEL_BUFFER,
        UNIFORM_BUFFER,
        STORAGE_BUFFER,
        UNIFORM_BUFFER_DYNAMIC,
        STORAGE_BUFFER_DYNAMIC,
        INPUT_ATTACHMENT
    };

    enum class CULL_MODE {
        NONE, FRONT, BACK, FRONT_AND_BACK = FRONT | BACK
    };

    enum class POLYGON_FRONT_FACE {
        COUNTER_CLOCKWISE, CLOCKWISE
    };

    enum class POLYGON_MODE {
        FILL, LINE, POINT
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
        ONE_MINUS_SRC1_ALPHA
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
        RGBA = R | G | B | A
    };
}
