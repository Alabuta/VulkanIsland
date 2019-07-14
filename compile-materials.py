import os
import re
import json

from subprocess import call
from typing import NamedTuple


class GLSLSettings(NamedTuple):
    version: int
    extensions: dict

class Shaders(NamedTuple):
    compiler_path: str
    source_path: str
    include_path: str
    file_extensions: tuple
    glslSettings: GLSLSettings
    vertex_attributes_layout: dict

class Materials(NamedTuple):
    source_path: str
    file_extensions: tuple


shaders = Shaders(
    'glslangValidator',
    './contents/shaders',
    './contents/shaders/include',
    ('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl'),
    GLSLSettings(
        460,
        {
            'GL_ARB_separate_shader_objects': 'enable',
            'GL_EXT_scalar_block_layout': 'enable'
        }
    ),
    {
        'POSITION': 0,
        'NORMAL': 1,
        'TEXCOORD_0': 2,
        'TEXCOORD_1': 3,
        'TANGENT': 4,
        'COLOR_0': 5,
        'JOINTS_0': 6,
        'WEIGHTS_0': 7,
    }
)

materials = Materials(
    './contents/materials',
    ('.json')
)

def preprocess_vertex_shader(material_data, shader_module):
    module_name = shader_module['name']
    shader_module_absolute_path = os.path.join(shaders.source_path, module_name + '.glsl')

    with open(shader_module_absolute_path, 'rb') as file:
        source_code = file.read().decode("utf-8")

        version_string = '#version {}\n'.format(shaders.glslSettings.version)

        extensions_string = ''

        for extension, behavior in shaders.glslSettings.extensions.items():
            extensions_string += '#extension {} : {}\n'.format(extension, behavior)

        extensions_string += '\n'

        vertex_attributes_string = ''

        for vertex_attribute in material_data['vertexAttributes']:
            semantic, type = [vertex_attribute[key] for key in ('semantic', 'type')]

            layout = shaders.vertex_attributes_layout[semantic]

            vertex_attributes_string += 'layout (location = {}) in {} {}\n'.format(layout, type, semantic)

        shader_header = '{}\n{}\n{}\n#line -1'.format(version_string, extensions_string, vertex_attributes_string)

        print(shader_header)

#def compile_shader(path):
#    ;


for root, dirs, material_relative_paths in os.walk(materials.source_path):
    for material_relative_path in material_relative_paths:
        if not material_relative_path.endswith(materials.file_extensions):
            continue

        print(material_relative_path)

        material_absolute_path = os.path.join(root, material_relative_path)

        with open(material_absolute_path, 'r') as json_file:
            material_data = json.load(json_file)
            
            shader_modules = material_data['shaderModules']

            vertex_stage_modules = [shader_module for shader_module in shader_modules if shader_module['stage'] == 'vertex']

            for vertex_stage_module in vertex_stage_modules:
                preprocess_vertex_shader(material_data, vertex_stage_module)

        break
                
