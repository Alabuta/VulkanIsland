__all__ = ['HLSLShaderPreprocessor']

import os
import re
from abc import ABC, abstractmethod

from operator import itemgetter
from functools import reduce

from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo
from abstract_shader_preprocessor import AbstractShaderPreprocessor


class HLSLShaderPreprocessor(AbstractShaderPreprocessor):
    def __init__(self, shaders_src_folder : str) -> None:
        self.__source_code=''

    @property
    def source_code(self) -> str:
        return self.__source_code

    @classmethod
    def process(self, shader_module : ShaderModuleInfo) -> None:
        # self.__shader_stage_header=self.__get_shader_stage_header(shader_module)

        # source_code_path=os.path.join(self.__shaders_src_folder, shader_module.source_name)
        # source_code=self.__get_shader_source_code(source_code_path)
        # assert source_code, f'can\'t get shader source code {source_code_path}'
        # source_code=GLSLShaderPreprocessor.__remove_inactive_techniques(shader_module.technique, source_code)

        # if shader_module.stage==ShaderStage.VERTEX:
        #     source_code=GLSLShaderPreprocessor.__sub_attributes_unpacks(source_code, shader_module.data)

        # if shader_module.constants:
        #     constants=GLSLShaderPreprocessor.__get_specialization_constants(shader_module.constants)
        #     source_code=f'{constants}\n{source_code}'

        # self.__source_code=f'{self.__shader_stage_header}\n#line 0\n{source_code}'
        self.__source_code='processed'