__all__ = ['GLSLShaderPreprocessor']

from operator import itemgetter, attrgetter
from functools import reduce, partial

from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class GLSLShaderPreprocessor:
    """Class for preprocessing GLSL shaders.

    Attributes:
    ----------
        language_version : int
            specifies a version of GLSL that should be used to compile/link a shader.
        language_extensions : list
            list of language extensions that have to be enabled.
    """

    def __init__(self, language_version, language_extensions) -> None:
        self.__language_version=language_version

        self.__version_line=f'#version {self.__language_version}\n'
        self.__extensions_lines=reduce(lambda s, e: s+f'#extension {e} : true\n', language_extensions, '')

    def process(self, shader_module):
        None

    def get_shader_stage_header(self, stage):
        """
        A function is used to get particular shade stage header lines.

        Parameters
        ----------
        stage : ShaderStage
            Specific shader stage
        """
        version_line, extensions_lines=self.__shader_directives
        include_directives='#include "vertex/vertex-attributes-unpack.glsl"\n';

        stage_inputs=shader_inputs[stage]

        return f'{version_line}\n{extensions_lines}\n{include_directives}\n{stage_inputs}\n#line 0'

    @property
    def shader_directives(self):
        """
        A function is used to get tuple of general shader directives.
        """
        return (self.__version_line, self.__extensions_lines)

    @property
    def shader_inputs(self):
        return 0
    
