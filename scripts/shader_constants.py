__all__ = ['ShaderStage']

from enum import Enum


class ShaderStage(Enum):
    VERTEX = 0,
    GEOMETRY = 1,
    TESS_CONTROL = 2,
    TESS_EVALUATION = 3,
    GEOMETRY = 4,
    FRAGMENT = 5,
    COMPUTE = 6