__all__ = ['GLSLShaderPreprocessor']

import os
import re
from functools import reduce
from operator import itemgetter

from abstract_shader_preprocessor import AbstractShaderPreprocessor
from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class GLSLShaderPreprocessor(AbstractShaderPreprocessor):
    """Class for preprocessing GLSL shaders.

    Attributes:
    ----------
        shaders_src_folder: str
            path to shaders' source files folder
        language_version: int
            specifies a version of GLSL that should be used to compile/link a shader.
    """

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

    LANGUAGE_EXTENSIONS = {
        'GL_ARB_separate_shader_objects': 'enable',
        'GL_EXT_shader_16bit_storage': 'enable',
        'GL_EXT_shader_8bit_storage': 'enable',
        # 'VK_KHR_shader_float16_int8 ': 'enable',
        'GL_EXT_scalar_block_layout': 'enable',
        'GL_GOOGLE_include_directive': 'enable'
    }

    def __init__(self, shaders_src_folder: str, language_version: str) -> None:
        self.__processed_shaders = {}

        self.__shader_stage_header = ''
        self.__source_code = ''

        self.__shaders_src_folder = shaders_src_folder
        self.__version_line = f'#version {language_version}\n'
        self.__extensions_lines = reduce(lambda s, e: s + f'#extension {e[0]}: {e[1]}\n',
                                         self.LANGUAGE_EXTENSIONS.items(), '')

    def __get_shader_stage_header(self, shader_module: ShaderModuleInfo) -> str:
        """
        A function is used to get particular shade stage header lines.

        Parameters
        ----------
        shader_module: ShaderModuleInfo
            Shader module info instance
        """
        stage_inputs = self.__shader_inputs(shader_module)

        include_directives = '#include "vertex/vertex-attributes-unpack.glsl"\n'

        return f'{self.__version_line}\n{self.__extensions_lines}\n{include_directives}\n{stage_inputs}'

    def __shader_inputs(self, shader_module: ShaderModuleInfo) -> str:
        if shader_module.stage == ShaderStage.VERTEX:
            return '\n'.join(map(lambda vl: f'layout (location = {self.VERTEX_ATTRIBUTES_LOCATIONS[vl["semantic"]]})\
             in {self.VERTEX_ATTRIBUTES_TYPES[vl["type"]]} {vl["semantic"]};', shader_module.data))

        elif shader_module.stage == ShaderStage.GEOMETRY:
            input_layout, output_layout, out_vertices_count = \
                itemgetter('inputLayout', 'outputLayout', 'outVerticesCount')(shader_module.data)

            return f'layout ({input_layout}) in;\nlayout ({output_layout}, max_vertices = {out_vertices_count}) out;\n'

        else:
            return ''

    @property
    def source_code(self) -> str:
        return self.__source_code

    def process(self, shader_module: ShaderModuleInfo) -> None:
        self.__shader_stage_header = self.__get_shader_stage_header(shader_module)

        source_code_path = os.path.join(self.__shaders_src_folder, shader_module.source_name)
        source_code = self.__get_shader_source_code(source_code_path)
        assert source_code, f'can\'t get shader source code {source_code_path}'
        source_code = GLSLShaderPreprocessor.__remove_inactive_techniques(shader_module.technique_index, source_code)

        if shader_module.stage == ShaderStage.VERTEX:
            source_code = GLSLShaderPreprocessor.__sub_attributes_unpacks(source_code, shader_module.data)

        if shader_module.constants:
            constants = GLSLShaderPreprocessor.__get_specialization_constants(shader_module.constants)
            source_code = f'{constants}\n{source_code}'

        self.__source_code = f'{self.__shader_stage_header}\n#line 0\n{source_code}'

    @staticmethod
    def __sub_techniques(source_code: str) -> str:
        pattern = r'([^\n]*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\([ |\t]*?(\d+)[ |\t]*?\)([^\n]*)'

        return re.sub(pattern, r'\1void technique\2()\3', source_code, 0, re.DOTALL)

    @staticmethod
    def __sub_attributes_unpacks(source_code, vertex_layout) -> str:
        for vertex_attribute in vertex_layout:
            semantic, attribute_type = itemgetter('semantic', 'type')(vertex_attribute)

            pattern = rf'unpackAttribute[ |\t]*\([ |\t]*?{semantic}[ |\t]*?\)'
            general_name = semantic.split('_')[0].lower()

            source_code = re.sub(pattern, f'unpackAttribute_{general_name}_{attribute_type}({semantic})', source_code,
                                 0, re.DOTALL)

        return source_code

    @staticmethod
    def __remove_inactive_techniques(technique_index: int, source_code: str) -> str:
        pattern = r'void technique[^I]\(\).*?{(.*?)}'
        pattern = pattern.replace('I', str(technique_index))

        while True:
            match = re.search(pattern, source_code, re.DOTALL)

            if not match:
                break

            substr = match.group(0)

            opening_brackets = substr.count('{')
            closing_brackets = substr.count('}')

            if opening_brackets == closing_brackets:
                source_code = source_code.replace(substr, '\n' * substr.count('\n'))

            else:
                assert opening_brackets > closing_brackets, 'opening brackets count less than closing ones'

                start, end = match.span()

                while True:
                    substr = source_code[end:]

                    match = re.search(r'.*?}', substr, re.DOTALL)

                    if not match:
                        break

                    end += match.span()[1]

                    opening_brackets += match.group(0).count('{')
                    closing_brackets += match.group(0).count('}')

                    if opening_brackets == closing_brackets:
                        substr=source_code[start:end]
                        source_code=source_code.replace(substr, '\n' * substr.count('\n'))

                        break

        return source_code

    @staticmethod
    def __get_specialization_constants(specialization_constants: list) -> str:
        constants = ''

        for index, specialization_constant in enumerate(specialization_constants):
            name, value, constant_type = itemgetter('name', 'value', 'type')(specialization_constant)

            constants += f'layout(constant_id = {index}) const {constant_type} {name} = {constant_type}({value});\n'

        return constants

    def __get_shader_source_code(self, path: str) -> str:
        if path in self.__processed_shaders:
            return self.__processed_shaders[path]

        with open(path, 'rb') as file:
            source_code = file.read().decode('UTF-8')

            source_code = AbstractShaderPreprocessor.remove_comments(source_code)
            source_code = GLSLShaderPreprocessor.__sub_techniques(source_code)

            self.__processed_shaders[path] = source_code

            return source_code
