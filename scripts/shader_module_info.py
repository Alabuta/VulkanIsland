__all__ = ['ShaderModuleInfo']


class ShaderModuleInfo:
    """Shader module info container.

    Attributes:
    ----------
        name : str
            shader source file name
        stage : ShaderStage
            shader stage type
        technique : int
            technique index
        constants : list
            array of json-like objects.
    """
    def __init__(self, name, stage, technique, constants) -> None:
        self.name=name
        self.stage=stage
        self.technique=technique
        self.constants=constants

    def __str__(self):
        return f'{self.name} {self.stage} {self.technique} {self.constants}'
