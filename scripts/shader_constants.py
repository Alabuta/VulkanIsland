__all__ = ['ShaderStage']

from enum import Enum


class ShaderStage(Enum):
    VERTEX = 0,
    TESS_CONTROL = 1,
    TESS_EVALUATION = 2,
    GEOMETRY = 3,
    FRAGMENT = 4,
    COMPUTE = 5

    @staticmethod
    def from_str(label):
        return {
            'vertex' : ShaderStage.VERTEX,
            'tess_ctrl' : ShaderStage.TESS_CONTROL,
            'tess_eval' : ShaderStage.TESS_EVALUATION,
            'geometry' : ShaderStage.GEOMETRY,
            'fragment' : ShaderStage.FRAGMENT,
            'compute' : ShaderStage.COMPUTE
        }[label];