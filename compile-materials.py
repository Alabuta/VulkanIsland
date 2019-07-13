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
    )
)

materials = Materials(
    './contents/materials',
    ('.json')
)

def preprocess_vertex_shader(shader_module):
    module_name = shader_module['name']
    shader_module_absolute_path = os.path.join(shaders.source_path, module_name + '.glsl')
    print(shader_module_absolute_path)

    with open(shader_module_absolute_path, 'r') as file:
        print(file)

#def compile_shader(path):
#    ;


for root, dirs, material_relative_paths in os.walk(materials.source_path):
    for material_relative_path in material_relative_paths:
        if not material_relative_path.endswith(materials.file_extensions):
            continue

        print(material_relative_path)

        material_absolute_path = os.path.join(root, material_relative_path)

        with open(material_absolute_path, 'r') as json_file:
            data = json.load(json_file)
            
            shader_modules = data['shaderModules']

            vertex_stage_modules = [shader_module for shader_module in shader_modules if shader_module['stage'] == 'vertex']
            print(vertex_stage_modules)

            for vertex_stage_module in vertex_stage_modules:
                preprocess_vertex_shader(vertex_stage_module)
                
