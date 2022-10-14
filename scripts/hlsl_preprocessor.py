__all__ = ['HLSLShaderPreprocessor']

import os
import re
from operator import itemgetter

from abstract_shader_preprocessor import AbstractShaderPreprocessor
from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class HLSLShaderPreprocessor(AbstractShaderPreprocessor):
    """Class for preprocessing HLSL shaders.

    Attributes:
    ----------
        shaders_src_folder: str
            path to shaders' source files folder
    """

    VERTEX_ATTRIBUTES_TYPES = {
        'r8i_norm': 'float',
        'rg8i_norm': 'float2',
        'rgb8i_norm': 'float3',
        'rgba8i_norm': 'float4',

        'r8ui_norm': 'float',
        'rg8ui_norm': 'float2',
        'rgb8ui_norm': 'float3',
        'rgba8ui_norm': 'float4',

        'r16i_norm': 'float',
        'rg16i_norm': 'float2',
        'rgb16i_norm': 'float3',
        'rgba16i_norm': 'float4',

        'r16ui_norm': 'float',
        'rg16ui_norm': 'float2',
        'rgb16ui_norm': 'float3',
        'rgba16ui_norm': 'float4',

        'r8i_scaled': 'float',
        'rg8i_scaled': 'float2',
        'rgb8i_scaled': 'float3',
        'rgba8i_scaled': 'float4',

        'r8ui_scaled': 'float',
        'rg8ui_scaled': 'float2',
        'rgb8ui_scaled': 'float3',
        'rgba8ui_scaled': 'float4',

        'r16i_scaled': 'float',
        'rg16i_scaled': 'float2',
        'rgb16i_scaled': 'float3',
        'rgba16i_scaled': 'float4',

        'r16ui_scaled': 'float',
        'rg16ui_scaled': 'float2',
        'rgb16ui_scaled': 'float3',
        'rgba16ui_scaled': 'float4',

        'r8i': 'int',
        'rg8i': 'int2',
        'rgb8i': 'int3',
        'rgba8i': 'int4',

        'r8ui': 'uint',
        'rg8ui': 'uint2',
        'rgb8ui': 'uint3',
        'rgba8ui': 'uint4',

        'r16i': 'int',
        'rg16i': 'int2',
        'rgb16i': 'int3',
        'rgba16i': 'int4',

        'r16ui': 'uint',
        'rg16ui': 'uint2',
        'rgb16ui': 'uint3',
        'rgba16ui': 'uint4',

        'r32i': 'int',
        'rg32i': 'int2',
        'rgb32i': 'int3',
        'rgba32i': 'int4',

        'r32ui': 'uint',
        'rg32ui': 'uint2',
        'rgb32ui': 'uint3',
        'rgba32ui': 'uint4',

        'r32f': 'float',
        'rg32f': 'float2',
        'rgb32f': 'float3',
        'rgba32f': 'float4',

        'r64f': 'double',
        'rg64f': 'double2',
        'rgb64f': 'double3',
        'rgba64f': 'double4'
    }

    def __init__(self, shaders_src_folder: str) -> None:
        self.__processed_shaders = {}
        self.__shaders_src_folder = shaders_src_folder
        self.__shader_stage_header = ''
        self.__source_code = ''

    @property
    def source_code(self) -> str:
        return self.__source_code

    def process(self, shader_module: ShaderModuleInfo) -> None:
        self.__shader_stage_header = self.__get_shader_stage_header(shader_module)

        source_code_path = os.path.join(self.__shaders_src_folder, shader_module.source_name)
        source_code = self.__get_shader_source_code(source_code_path)
        assert source_code, f'can\'t get shader source code {source_code_path}'

        source_code = HLSLShaderPreprocessor.__remove_inactive_techniques(shader_module.technique_index, source_code)

        if shader_module.stage == ShaderStage.VERTEX:
            source_code = HLSLShaderPreprocessor.__sub_attributes_unpacks(source_code, shader_module.data)

        if shader_module.constants:
            constants = HLSLShaderPreprocessor.__get_specialization_constants(shader_module.constants)
            source_code = f'{constants}\n{source_code}'

        self.__source_code = f'{self.__shader_stage_header}\n#line 0\n{source_code}'

    def __get_shader_stage_header(self, shader_module: ShaderModuleInfo) -> str:
        """
        A function is used to get particular shade stage header lines.

        Parameters
        ----------
        shader_module: ShaderModuleInfo
            Shader module info instance
        """
        stage_inputs = self.__shader_inputs(shader_module)

        include_directives = '#include "vertex/vertex-attributes-unpack.hlsl"\n';

        return f'{include_directives}\n{stage_inputs}'
        # return f'{self.__version_line}\n{self.__extensions_lines}\n{include_directives}\n{stage_inputs}'

    def __shader_inputs(self, shader_module: ShaderModuleInfo) -> str:
        if shader_module.stage == ShaderStage.VERTEX:
            attributes = '\n'.join(map(lambda
                                           vl: f'\t[[vk::location({self.VERTEX_ATTRIBUTES_LOCATIONS[vl["semantic"]]})]] {self.VERTEX_ATTRIBUTES_TYPES[vl["type"]]} {vl["semantic"]}: {re.sub("_", "", vl["semantic"])};',
                                       shader_module.data))
            return 'struct VS_INPUT\n{\n' + attributes + '\n};'

        # elif shader_module.stage= = ShaderStage.GEOMETRY:
        #     input_layout, output_layout, out_vertices_count = itemgetter('inputLayout', 'outputLayout', 'outVerticesCount')(shader_module.data)
        #     return f'layout ({input_layout}) in;\nlayout ({output_layout}, max_vertices = {out_vertices_count}) out;\n'

        else:
            return ''

    @staticmethod
    def __remove_inactive_techniques(technique_index: int, source_code: str) -> str:
        p0 = r'([^\[]\[[^}]+?\][^\]]\n*?)*?'
        p1 = r'[^\n]*[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\([ |\t]*?[^I][ |\t]*?\)'.replace('I',
                                                                                                   str(technique_index))
        p2 = r'\s*?.*?[\w\d]+?[ |\t]*?\(.*?\).*?\s*?{\s*[\s\S]*?\n*?}'
        pattern = f'({p0}{p1}{p2})'

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
                        substr = source_code[start:end]
                        source_code = source_code.replace(substr, '\n' * substr.count('\n'))

                        break

        return source_code

    @staticmethod
    def __sub_attributes_unpacks(source_code, vertex_layout) -> str:
        for vertex_attribute in vertex_layout:
            semantic, type = itemgetter('semantic', 'type')(vertex_attribute)

            pattern = rf'unpackAttribute[ |\t]*\([ |\t]*?(\w*?\s*?\.\s*?){semantic}(.*?)[ |\t]*?\)'
            general_name = semantic.split('_')[0].lower()

            source_code = re.sub(pattern, rf'unpackAttribute_{general_name}_{type}(\1{semantic}\2)', source_code, 0,
                                 re.DOTALL)

        return source_code

    @staticmethod
    def __get_specialization_constants(specialization_constants: list) -> str:
        constants = ''

        for index, specialization_constant in enumerate(specialization_constants):
            name, value, type = itemgetter('name', 'value', 'type')(specialization_constant)

            constants += f'[[vk::constant_id({index})]] const {type} {name} = {type}({value});\n'

        return constants

    def __get_shader_source_code(self, path: str) -> str:
        if path in self.__processed_shaders:
            return self.__processed_shaders[path]

        with open(path, 'rb') as file:
            source_code = file.read().decode('UTF-8')

            source_code = AbstractShaderPreprocessor.remove_comments(source_code)

            self.__processed_shaders[path] = source_code

            return source_code