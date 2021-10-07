__all__ = ['MaterialTechnique']

from operator import itemgetter

from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class MaterialTechnique:
    """Class-container for a particular material technique, that contains vertex layouts, primitive inputs and etc.

    Attributes:
    ----------
        material_description : object
            json-like object populated by material techiques, shader modules, vertex layouts, primitive inputs and so forth.
        technique_index : int
            technique index in material description json-like object
    """
    def __init__(self, material_description : object, technique_index : int) -> None:
        self.__material_description=material_description
        self.__technique_index=technique_index

        techniques, shader_modules, vertex_attributes=itemgetter('techniques', 'shaderModules', 'vertexAttributes')(self.__material_description)
        primitive_topologies=self.__material_description.get('primitiveTopologies', [])

        shader_bundle, vertex_layouts=itemgetter('shaderBundle', 'vertexLayouts')(techniques[self.__technique_index])
        primitive_inputs=techniques[self.__technique_index].get('primitiveInputs', [])

        self.__vertex_layouts=list(map(lambda vl: [vertex_attributes[i] for i in vl], vertex_layouts))

        self.__primitive_inputs=[primitive_topologies[i] for i in primitive_inputs]

        self.__shader_bundle=[]
        for shader_module in shader_bundle:
            shader_index, technique_index=itemgetter('index', 'technique', )(shader_module)
            constants=shader_module.get('constants', [])
            name, stage=itemgetter('name', 'stage')(shader_modules[shader_index])
            stage=ShaderStage.from_str(stage)

            if stage==ShaderStage.VERTEX:
                for vertex_layout in self.__vertex_layouts:
                    self.__shader_bundle.append(ShaderModuleInfo(name, stage, technique_index, constants, vertex_layout))

            elif stage==ShaderStage.GEOMETRY:
                for primitive_input in self.__primitive_inputs:
                    self.__shader_bundle.append(ShaderModuleInfo(name, stage, technique_index, constants, primitive_input))

            else:
                self.__shader_bundle.append(ShaderModuleInfo(name, stage, technique_index, constants))

    @property
    def shader_bundle(self) -> list:
        return self.__shader_bundle