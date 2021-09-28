__all__ = ['ShaderModuleInfo']

from operator import itemgetter, attrgetter
from functools import reduce, partial

from shader_constants import ShaderStage


class ShaderModuleInfo:
    """Shader module info container.

    Attributes:
    ----------
        name : str
            shader source file name
        stage : ShaderStage
            shader stage type
        technique : int
            technique index
        constants : list
            array of json-like objects
        data : object
            any data that specific to the stage
    """
    def __init__(self, name, stage, technique, constants, data=None) -> None:
        self.name=name
        self.stage=stage
        self.technique=technique
        self.constants=constants
        self.__data=data

    def __compile_vertex_layout_name(self):
        getter=itemgetter('semantic','type')
        return '|'.join(map(lambda a: ':'.join(getter(a)).lower(), self.__data))

    def __str__(self):
        return {
            ShaderStage.VERTEX : f'{self.name}.{self.technique}.{self.__compile_vertex_layout_name()}',
            ShaderStage.TESS_CONTROL : f'{self.name}.{self.technique}',
            ShaderStage.TESS_EVALUATION : f'{self.name}.{self.technique}',
            ShaderStage.GEOMETRY : f'{self.name}.{self.technique}.{self.__data}',
            ShaderStage.FRAGMENT : f'{self.name}.{self.technique}',
            ShaderStage.COMPUTE : f'{self.name}.{self.technique}'
        }[self.stage];
        # return f'{self.name} {self.stage} {self.technique} {self.constants}'
