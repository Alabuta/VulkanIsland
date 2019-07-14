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

    version_string = '#version {}\n'.format(shaders.glslSettings.version)

    extensions_string = ''

    for extension, behavior in shaders.glslSettings.extensions.items():
        extensions_string += '#extension {} : {}\n'.format(extension, behavior)

    extensions_string += '\n'

    vertex_attributes_string = ''

    for vertex_attribute in material_data['vertexAttributes']:
        semantic, type = [vertex_attribute[key] for key in ('semantic', 'type')]

        layout = shaders.vertex_attributes_layout[semantic]

        vertex_attributes_string += 'layout (location = {}) in {} {};\n'.format(layout, type, semantic)

    shader_header = '{}\n{}\n{}\n#line 0'.format(version_string, extensions_string, vertex_attributes_string)

    source_code = ''

    with open(shader_module_absolute_path, 'r') as file:
        #source_code = file.read().decode("utf-8")

        for line in file:
            line = re.sub(
                r'[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)',
                lambda match_object : 'void technique{}()'.format(match_object.group(1)),
                #lambda match_object : 'void technique_{}()'.format(chr(ord('a') + int(match_object.group(1)))),
                line
            )

            source_code += line

    for technique in re.findall(r'void (technique\d)\(\)', source_code):
        full_source_code = '{}\n{}'.format(shader_header, source_code)

        source_path = os.path.join(shaders.source_path, module_name)

        with open(source_path, 'w+') as file:
            file.write(full_source_code)

        entry_point = technique
        output_path = os.path.join(shaders.source_path, '{}.{}.spv'.format(module_name, entry_point))
        print(entry_point, output_path)

        call([shaders.compiler_path, '-V', '-I' + shaders.include_path, '-e', entry_point, '-o', output_path, source_path])

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
                
