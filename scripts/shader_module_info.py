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
        self.__name=name
        self.__stage=stage
        self.__technique=technique
        self.__constants=constants
        self.__data=data

    @property
    def name(self):
        return self.__name

    @property
    def stage(self):
        return self.__stage

    @property
    def technique(self):
        return self.__technique

    @property
    def constants(self):
        return self.__constants

    @property
    def data(self):
        return self.__data

    def __str__(self):
        s=f'{self.__name}.{self.__technique}'

        if self.__stage==ShaderStage.VERTEX:
            getter=itemgetter('semantic','type')
            vertex_layout='|'.join(map(lambda a: ':'.join(getter(a)).lower(), self.__data))
            s+=f'.{vertex_layout}'

        elif self.__stage==ShaderStage.GEOMETRY:
            primitive_input=self.__data['inputLayout']
            s+=f'.{primitive_input}'

        return s
