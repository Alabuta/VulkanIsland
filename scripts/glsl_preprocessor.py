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
        header=self.get_shader_stage_header(shader_module)

    def get_shader_stage_header(self, shader_module):
        """
        A function is used to get particular shade stage header lines.

        Parameters
        ----------
        stage : ShaderStage
            Specific shader stage
        """
        version_line, extensions_lines=self.shader_directives
        include_directives='#include "vertex/vertex-attributes-unpack.glsl"\n';

        stage_inputs=self.__shader_inputs(shader_module)

        return f'{version_line}\n{extensions_lines}\n{include_directives}\n{stage_inputs}\n#line 0'

    @property
    def shader_directives(self):
        """
        A function is used to get tuple of general shader directives.
        """
        return (self.__version_line, self.__extensions_lines)

    def __shader_inputs(self, shader_module):
        if shader_module.stage==ShaderStage.VERTEX:
            getter=itemgetter('semantic','type')
            vertex_layout='\n'.join(map(lambda a: ':'.join(getter(a)).lower(), self.__data))
            s+=f'.{vertex_layout}'

        elif shader_module.stage==ShaderStage.GEOMETRY:
            input_layout, output_layout, out_vertices_count=itemgetter('inputLayout', 'outputLayout', 'outVerticesCount')(shader_module.data)
            return f'layout ({input_layout}) in;\nlayout ({output_layout}, max_vertices = {out_vertices_count}) out;\n'

        else:
            return ''
    
