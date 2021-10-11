__all__ = ['AbstractShaderPreprocessor']

import re

from abc import ABC, abstractmethod

from shader_module_info import ShaderModuleInfo


class AbstractShaderPreprocessor(ABC):
    @property
    @abstractmethod
    def source_code(cls):
        raise NotImplementedError

    @classmethod
    @abstractmethod
    def process(cls, shader_module : ShaderModuleInfo):
        raise NotImplementedError

    @staticmethod
    def remove_comments(source_code : str) -> str:
        pattern=r'(?://[^\n]*|/\*(?:(?!\*/).)*\*/)'

        substrs=re.findall(pattern, source_code, re.DOTALL)
        
        for substr in substrs:
            source_code=source_code.replace(substr, '\n' * substr.count('\n'))

        return source_code
