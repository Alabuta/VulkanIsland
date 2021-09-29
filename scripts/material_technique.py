__all__ = ['MaterialTechnique']

from operator import itemgetter, attrgetter
from functools import reduce, partial

from shader_constants import ShaderStage
from shader_module_info import ShaderModuleInfo


class MaterialTechnique:
    """Class-container for a particular material technique, that contains vertex layouts, primitive inputs and etc.

    Attributes:
    ----------
        material_description : object
            json-like object populated by material techiques, shader modules, vertex layouts, primitive inputs and so forth.
    """
    def __init__(self, material_description, technique_index) -> None:
        self.__material_description=material_description
        self.__technique_index=technique_index

    # def build(self):
        techniques, shader_modules, vertex_attributes=itemgetter('techniques', 'shaderModules', 'vertexAttributes')(self.__material_description)
        primitive_topologies=self.__material_description.get('primitiveTopologies', [])

        shader_bundle, vertex_layouts, primitive_inputs=itemgetter('shaderBundle', 'vertexLayouts', 'primitiveInputs')(techniques[self.__technique_index])

        self.__vertex_layouts=list(map(lambda vl: [vertex_attributes[i] for i in vl], vertex_layouts))

        self.__primitive_inputs=[primitive_topologies[i] for i in primitive_inputs]

        # shader_bundle=map(lambda s: (shader_modules[s['index']], s), shader_bundle)
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


        # self.__shader_bundle=list(map(lambda s: ShaderModuleInfo(s[0]['name'], ShaderStage.from_str(s[0]['stage']), s[1]['technique'], s[1].get('constants', [])), shader_bundle))

    @property
    def shader_bundle(self):
        return self.__shader_bundle

    @property
    def vertex_layouts(self):
        return self.__vertex_layouts

    @property
    def primitive_inputs(self):
        return self.__primitive_inputs

    @property
    def vertex_stage_shader_modules(self):
        return filter(lambda sm: sm.stage == ShaderStage.VERTEX, self.__shader_bundle)

    @property
    def geometry_stage_shader_modules(self):
        return filter(lambda sm: sm.stage == ShaderStage.GEOMETRY, self.__shader_bundle)

    @property
    def fragment_stage_shader_modules(self):
        return filter(lambda sm: sm.stage == ShaderStage.FRAGMENT, self.__shader_bundle)


        # self.vertex_layouts=[vertex_attributes[i] for i in vertex_layout]
        # self.shader_modules=(
        #     ShaderStage.VERTEX : 
        # )
        # vertex_attribute=vertex_attributes[vertex_attribute_index]
        # semantic, type=itemgetter('semantic', 'type')(vertex_attribute)