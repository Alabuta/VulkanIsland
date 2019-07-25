import os
import re
import json
import subprocess
# import pyparsing

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
    vertex_attributes_locations: dict
    stage2extension: dict

class Materials(NamedTuple):
    source_path: str
    file_extensions: tuple


shaders = Shaders(
    compiler_path = 'glslangValidator',
    source_path = './contents/shaders',
    include_path = './contents/shaders/include',
    file_extensions = ('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl'),
    glslSettings = GLSLSettings(
        version = 460,
        extensions = {
            'GL_ARB_separate_shader_objects': 'enable',
            'GL_EXT_scalar_block_layout': 'enable'
        }
    ),
    vertex_attributes_locations = {
        'POSITION': 0,
        'NORMAL': 1,
        'TEXCOORD_0': 2,
        'TEXCOORD_1': 3,
        'TANGENT': 4,
        'COLOR_0': 5,
        'JOINTS_0': 6,
        'WEIGHTS_0': 7
    },
    stage2extension = {
        'vertex': 'vert',
        'tesselation_control': 'tesc',
        'tesselation_evaluation': 'tese',
        'geometry': 'geom',
        'fragment': 'frag',
        'compute': 'comp'
    }
)

materials = Materials(
    source_path = './contents/materials',
    file_extensions = ('.json')
)


def shader_directives():
    version_line = f'#version {shaders.glslSettings.version}\n'

    extensions_lines = ''

    for extension, behavior in shaders.glslSettings.extensions.items():
        extensions_lines +=  f'#extension {extension} : {behavior}\n'

    return (version_line, extensions_lines)


def shader_header(stage, shader_inputs):
    version_line, extensions_lines = shader_directives()

    stage_inputs = shader_inputs[stage]

    return f'{version_line}\n{extensions_lines}\n{stage_inputs}\n#line 0'


def shader_vertex_attribute_layout(vertex_attributes, technique):
    vertex_attribute_layout = [ ]

    for vertex_attribute_index in technique['vertexLayout']:
        vertex_attribute = vertex_attributes[vertex_attribute_index]

        semantic, type = [ vertex_attribute[k] for k in ('semantic', 'type') ]

        vertex_attribute_layout.append([ semantic, type ])

    vertex_attributes_lines = ''

    for vertex_attribute in vertex_attribute_layout:
        semantic, attribute_type = vertex_attribute

        location = shaders.vertex_attributes_locations[semantic]

        vertex_attributes_lines +=  f'layout (location = {location}) in {attribute_type} {semantic};\n'

    return vertex_attributes_lines


# TODO:: add another shader input structures
def shader_inputs(material, technique):
    vertex_attributes = material['vertexAttributes']

    vertex_attributes_lines = shader_vertex_attribute_layout(vertex_attributes, technique)

    vertex_stage_inputs = f'{vertex_attributes_lines}\n'
    tesc_stage_inputs = f'\n'
    tese_stage_inputs = f'\n'
    geometry_stage_inputs = f'\n'
    fragment_stage_inputs = f'\n'
    compute_stage_inputs = f'\n'

    inputs = {
        'vertex': vertex_stage_inputs,
        'tesselation_control': tesc_stage_inputs,
        'tesselation_evaluation': tese_stage_inputs,
        'geometry': geometry_stage_inputs,
        'fragment': fragment_stage_inputs,
        'compute': compute_stage_inputs
    }

    return inputs


def remove_comments(source_code):
    pattern = r'(?://[^\n]*|/\*(?:(?!\*/).)*\*/)'

    return re.sub(pattern, '', source_code, 0, re.DOTALL)


def sub_techniques(source_code):
    pattern = r'([^\n]*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)([^\n]*)'

    return re.sub(pattern, r'\1void technique\2()\3', source_code, 0, re.DOTALL)
    

def remove_inactive_techniques(source_code):
    pattern = r'(?:void technique.+?\(\))'


def compile_material(material_data):
    techniques, shader_modules = [
        material_data[k] for k in ('techniques', 'shaderModules')
    ]
    
    for technique_index, technique in enumerate(techniques):
        technique_name = f'technique{technique_index}'

        inputs = shader_inputs(material_data, technique)

        for shader_module_index in technique['shadersBundle']:
            shader_module = shader_modules[shader_module_index]

            name, stage = shader_module['name'], shader_module['stage']

            header = shader_header(stage, inputs)

            source_code = ''

            with open(os.path.join(shaders.source_path, f'{name}.glsl'), 'rb') as file:
                source_code = file.read().decode('UTF-8')

                source_code = remove_comments(source_code)
                source_code = sub_techniques(source_code)

                source_code = f'{header}\n{source_code}'

                output_path = os.path.join(shaders.source_path, f'{name}.{technique_name}.spv')

                compiler = subprocess.Popen([
                    shaders.compiler_path,
                    '-e', technique_name,
                    '--source-entrypoint', 'main',
                    '-V',
                    # '-H',
                    '--target-env', 'vulkan1.1',
                    f'-I{shaders.include_path}',
                    '-o', output_path,
                    '--stdin',
                    '-S', shaders.stage2extension[stage]
                ], stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE)

                output, errors = compiler.communicate(source_code.encode('UTF-8'))

                print(output.decode('UTF-8'))

                if compiler.returncode !=  0:
                    print(errors.decode('UTF-8'))


for root, dirs, material_relative_paths in os.walk(materials.source_path):
    for material_relative_path in material_relative_paths:
        if not material_relative_path.endswith(materials.file_extensions):
            continue

        material_absolute_path = os.path.join(root, material_relative_path)

        with open(material_absolute_path, 'r') as json_file:
            compile_material(json.load(json_file))
            break
