__all__ = ['AbstractShaderPreprocessor']

import re

from abc import ABC, abstractmethod

from shader_module_info import ShaderModuleInfo


class AbstractShaderPreprocessor(ABC):
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
