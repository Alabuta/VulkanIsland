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
    """Class for preprocessing HLSL shaders.

    Attributes:
    ----------
        shaders_src_folder : str
            path to shaders' source files folder
    """
    def __init__(self, shaders_src_folder : str) -> None:
        self.__processed_shaders={}
        self.__shaders_src_folder=shaders_src_folder

    @property
    def source_code(self) -> str:
        return self.__source_code

    def process(self, shader_module : ShaderModuleInfo) -> None:
        source_code_path=os.path.join(self.__shaders_src_folder, shader_module.source_name)
        source_code=self.__get_shader_source_code(source_code_path)
        assert source_code, f'can\'t get shader source code {source_code_path}'

        self.__source_code=f'{source_code}'

    def __get_shader_source_code(self, path : str) -> str:
        if path in self.__processed_shaders:
            return self.__processed_shaders[path]

        with open(path, 'rb') as file:
            source_code=file.read().decode('UTF-8')

            # source_code=GLSLShaderPreprocessor.__remove_comments(source_code)
            # source_code=GLSLShaderPreprocessor.__sub_techniques(source_code)

            self.__processed_shaders[path]=source_code

            return source_code