__all__ = ['MaterialTechnique']

from operator import itemgetter, attrgetter
from functools import reduce, partial

from shader_constants import ShaderStage


class ShaderModule:
    """Shader module data container.

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

class MaterialTechnique:
    """Class-container for a particular material technique, that contains vertex layouts, primitive inputs and etc.

    Attributes:
    ----------
        material_description : object
            json-like object populated by material techiques, shader modules, vertex layouts, primitive inputs and so forth.
    """
    def __init__(self, material_description, technique_index) -> None:
        self.material_description=material_description
        self.technique_index=technique_index

    # def build(self):
        techniques, shader_modules, vertex_attributes=itemgetter('techniques', 'shaderModules', 'vertexAttributes')(self.material_description)
        primitive_topologies=self.material_description.get('primitiveTopologies', [])

        shader_bundles, vertex_layouts, primitive_inputs=itemgetter('shaderBundle', 'vertexLayouts', 'primitiveInputs')(techniques[self.technique_index])

        shb=list(map(lambda s: (shader_modules[s['index']], s), shader_bundles))
        self.shader_bundle=list(map(lambda s: ShaderModule(s[0]['name'], ShaderStage.from_str(s[0]['stage']), s[1]['technique'], s[1].get('constants', [])), shb))
        # print(shb)
        for sm in self.shader_bundle:
            print(f'{sm}')

        # self.shader_bundle=list(map(lambda sm: (sm['name'], ShaderStage.from_str(sm['stage'])), shader_bundle))

        self.vertex_layouts=list(map(lambda vl: [vertex_attributes[i] for i in vl], vertex_layouts))

        self.primitive_inputs=[primitive_topologies[i] for i in primitive_inputs]

    @property
    def shader_bundle(self):
        return self.shader_bundle

    @property
    def vertex_layouts(self):
        return self.vertex_layouts

    @property
    def primitive_inputs(self):
        return self.primitive_inputs


        # self.vertex_layouts=[vertex_attributes[i] for i in vertex_layout]
        # self.shader_modules=(
        #     ShaderStage.VERTEX : 
        # )
        # vertex_attribute=vertex_attributes[vertex_attribute_index]
        # semantic, type=itemgetter('semantic', 'type')(vertex_attribute)