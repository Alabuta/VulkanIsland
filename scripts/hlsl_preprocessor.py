__all__ = ['HLSLShaderPreprocessor']

import os
import re
import abc

from operator import itemgetter
from functools import reduce

from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class ShaderPreprocessorInterface(metaclass=abc.ABCMeta):
    @classmethod
    def __subclasshook__(cls, subclass):
        return (hasattr(subclass, 'process') and 
                callable(subclass.process) and 
                hasattr(subclass, 'source_code') and 
                callable(subclass.source_code) or 
                NotImplemented)

    @abc.abstractmethod
    def process(self, shader_module : ShaderModuleInfo):
        raise NotImplementedError

    @abc.abstractmethod
    def source_code(self):
        raise NotImplementedError


class HLSLShaderPreprocessor:
    def __init__(self) -> None:
        pass