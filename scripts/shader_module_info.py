__all__ = ['ShaderLanguage', 'ShaderModuleInfo']

from enum import Enum
from operator import itemgetter

from shader_constants import ShaderStage


class ShaderLanguage(Enum):
    GLSL = 0,
    HLSL = 1

    @staticmethod
    def from_str(label):
        return {
            '.glsl' : ShaderLanguage.GLSL,
            '.hlsl' : ShaderLanguage.HLSL
        }[label];


class ShaderModuleInfo:
    """Shader module info container.

    Attributes:
    ----------
        language : ShaderLanguage
            shader language
        name : str
            shader source file name
        stage : ShaderStage
            shader stage type
        technique_index : int
            technique index
        constants : list
            array of json-like objects
        data : object
            any data that specific to the stage
    """
    def __init__(self, shader_language : ShaderLanguage, name : str, stage : ShaderStage, technique_index : int, constants : list, data=None) -> None:
        self.__shader_language=shader_language
        self.__name=name
        self.__stage=stage
        self.__technique_index=technique_index
        self.__constants=constants
        self.__data=data

    @property
    def shader_language(self) -> ShaderLanguage:
        return self.__shader_language

    @property
    def source_name(self) -> str:
        return self.__name

    @property
    def stage(self) -> ShaderStage:
        return self.__stage

    @property
    def technique_index(self) -> int:
        return self.__technique_index

    @property
    def constants(self) -> list:
        return self.__constants

    @property
    def data(self) -> object:
        return self.__data

    @property
    def entry_point(self) -> str:
        return f'technique{self.__technique_index}';

    @property
    def target_name(self) -> str:
        s=f'{self.__name}.{self.__technique_index}'

        if self.__stage==ShaderStage.VERTEX:
            getter=itemgetter('semantic','type')
            vertex_layout='|'.join(map(lambda a: ':'.join(getter(a)).lower(), self.__data))
            s+=f'.{vertex_layout}'

        elif self.__stage==ShaderStage.GEOMETRY:
            primitive_input=self.__data['inputLayout']
            s+=f'.{primitive_input}'

        return s
