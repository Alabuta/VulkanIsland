__all__ = ['AbstractShaderPreprocessor']

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
