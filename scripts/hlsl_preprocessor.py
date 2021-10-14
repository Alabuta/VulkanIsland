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
        
        source_code=HLSLShaderPreprocessor.__remove_inactive_techniques(shader_module.technique_index, source_code)
        print(source_code)

        self.__source_code=f'{source_code}'
    
    @staticmethod
    def __remove_inactive_techniques(technique_index : int, source_code : str) -> str:
        # pattern=r'void technique[^I]\(\).*?{(.*?)}'
        pattern=r'[^\n]*[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\([ |\t]*?\d+?[ |\t]*?\)\n+(?:.*?]\n)*[\w\d]+?[ |\t]+?[\w\d]+?[ |\t]*?\(.*?\).*?\s*?'
        # pattern=pattern.replace('I', str(technique_index))
        print(pattern)


        while True:
            match=re.search(pattern, source_code, re.DOTALL)

            if not match:
                break
        
            substr=match.group(0)
            print(1111111111111)
            continue

            opening_brackets=substr.count('{')
            closing_brackets=substr.count('}')

            if opening_brackets == closing_brackets:
                source_code=source_code.replace(substr, '\n' * substr.count('\n'))

            else:
                assert opening_brackets > closing_brackets, 'opening brackets count less than closing ones'

                start, end=match.span()

                while True:
                    substr=source_code[end:]

                    match=re.search(r'.*?}', substr, re.DOTALL)

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

    def __get_shader_source_code(self, path : str) -> str:
        if path in self.__processed_shaders:
            return self.__processed_shaders[path]

        with open(path, 'rb') as file:
            source_code=file.read().decode('UTF-8')

            source_code=AbstractShaderPreprocessor.remove_comments(source_code)

            self.__processed_shaders[path]=source_code

            return source_code