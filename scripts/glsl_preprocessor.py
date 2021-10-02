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
    VERTEX_ATTRIBUTES_LOCATIONS={
        'POSITION': 0,
        'NORMAL': 1,
        'TEXCOORD_0': 2,
        'TEXCOORD_1': 3,
        'TANGENT': 4,
        'COLOR_0': 5,
        'JOINTS_0': 6,
        'WEIGHTS_0': 7
    }

    VERTEX_ATTRIBUTES_TYPES={
        'r8i_norm': 'float',
        'rg8i_norm': 'vec2',
        'rgb8i_norm': 'vec3',
        'rgba8i_norm': 'vec4',

        'r8ui_norm': 'float',
        'rg8ui_norm': 'vec2',
        'rgb8ui_norm': 'vec3',
        'rgba8ui_norm': 'vec4',

        'r16i_norm': 'float',
        'rg16i_norm': 'vec2',
        'rgb16i_norm': 'vec3',
        'rgba16i_norm': 'vec4',

        'r16ui_norm': 'float',
        'rg16ui_norm': 'vec2',
        'rgb16ui_norm': 'vec3',
        'rgba16ui_norm': 'vec4',

        'r8i_scaled': 'float',
        'rg8i_scaled': 'vec2',
        'rgb8i_scaled': 'vec3',
        'rgba8i_scaled': 'vec4',

        'r8ui_scaled': 'float',
        'rg8ui_scaled': 'vec2',
        'rgb8ui_scaled': 'vec3',
        'rgba8ui_scaled': 'vec4',

        'r16i_scaled': 'float',
        'rg16i_scaled': 'vec2',
        'rgb16i_scaled': 'vec3',
        'rgba16i_scaled': 'vec4',

        'r16ui_scaled': 'float',
        'rg16ui_scaled': 'vec2',
        'rgb16ui_scaled': 'vec3',
        'rgba16ui_scaled': 'vec4',

        'r8i': 'int',
        'rg8i': 'ivec2',
        'rgb8i': 'ivec3',
        'rgba8i': 'ivec4',

        'r8ui': 'uint',
        'rg8ui': 'uvec2',
        'rgb8ui': 'uvec3',
        'rgba8ui': 'uvec4',

        'r16i': 'int',
        'rg16i': 'ivec2',
        'rgb16i': 'ivec3',
        'rgba16i': 'ivec4',

        'r16ui': 'uint',
        'rg16ui': 'uvec2',
        'rgb16ui': 'uvec3',
        'rgba16ui': 'uvec4',

        'r32i': 'int',
        'rg32i': 'ivec2',
        'rgb32i': 'ivec3',
        'rgba32i': 'ivec4',

        'r32ui': 'uint',
        'rg32ui': 'uvec2',
        'rgb32ui': 'uvec3',
        'rgba32ui': 'uvec4',

        'r32f': 'float',
        'rg32f': 'vec2',
        'rgb32f': 'vec3',
        'rgba32f': 'vec4',

        'r64f': 'double',
        'rg64f': 'dvec2',
        'rgb64f': 'dvec3',
        'rgba64f': 'dvec4'
    }

    def __init__(self, language_version, language_extensions) -> None:

        self.__language_version=language_version

        self.__version_line=f'#version {self.__language_version}\n'
        self.__extensions_lines=reduce(lambda s, e: s+f'#extension {e} : true\n', language_extensions, '')

    def process(self, shader_module):
        self.header=self.get_shader_stage_header(shader_module)

    def get_shader_stage_header(self, shader_module):
        """
        A function is used to get particular shade stage header lines.

        Parameters
        ----------
        shader_module : ShaderModuleInfo
            Shader module info instance
        """
        stage_inputs=self.__shader_inputs(shader_module)

        version_line, extensions_lines=self.shader_directives
        include_directives='#include "vertex/vertex-attributes-unpack.glsl"\n';

        return f'{version_line}\n{extensions_lines}\n{include_directives}\n{stage_inputs}\n#line 0'

    @property
    def shader_directives(self):
        """
        A function is used to get tuple of general shader directives.
        """
        return (self.__version_line, self.__extensions_lines)

    def __shader_inputs(self, shader_module):
        if shader_module.stage==ShaderStage.VERTEX:
            return '\n'.join(map(lambda vl: f'layout (location = {self.VERTEX_ATTRIBUTES_LOCATIONS[vl["semantic"]]}) in {self.VERTEX_ATTRIBUTES_TYPES[vl["type"]]} {vl["semantic"]};', shader_module.data))

        elif shader_module.stage==ShaderStage.GEOMETRY:
            input_layout, output_layout, out_vertices_count=itemgetter('inputLayout', 'outputLayout', 'outVerticesCount')(shader_module.data)
            return f'layout ({input_layout}) in;\nlayout ({output_layout}, max_vertices = {out_vertices_count}) out;\n'

        else:
            return ''
    
