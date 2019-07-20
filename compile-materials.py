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
        'WEIGHTS_0': 7,
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
        extensions_lines += f'#extension {extension} : {behavior}\n'

    return (version_line, extensions_lines)


def shader_header(shader_inputs):
    version_line, extensions_lines = shader_directives()

    return f'{version_line}\n{extensions_lines}\n{shader_inputs}\n#line 0'


def shader_vertex_attribute_layout(vertex_attributes, technique):
    vertex_attribute_layout = [ ]

    for vertex_attribute_index in technique['vertexLayout']:
        vertex_attribute = vertex_attributes[vertex_attribute_index]

        semantic, type = [ vertex_attribute[k] for k in ('semantic', 'type') ]

        vertex_attribute_layout.append([ semantic, type ])

    return vertex_attribute_layout


# TODO:: add another shader input structures
def shader_inputs(vertex_attribute_layout):
    vertex_attributes_lines = ''

    for vertex_attribute in vertex_attribute_layout:
        semantic, attribute_type = vertex_attribute

        location = shaders.vertex_attributes_locations[semantic]

        vertex_attributes_lines += f'layout (location = {location}) in {attribute_type} {semantic};\n'

    return vertex_attributes_lines


def remove_one_line_comments(source_code):
    unprocessed = source_code
    processed = ''

    for row in unprocessed.splitlines(True):
        found = re.findall(r'(?<=[^/])/\*.+?\*/', row)

        if found:
            print(found)
            processed += f'\n'

        else:
            processed += row

    #processed, unprocessed = '', processed

    #for row in unprocessed.splitlines(True):
    #    found = re.findall(r'.*?(?=/{2})', row)

    #    if found:
    #        processed += f'{found[0]}\n'

    #    else:
    #        processed += row

    return processed


def remove_multi_line_comments(source_code):
    rows = ''

    for row in source_code:
        found = re.findall(r'.+?(?=/\*)', row)

        if found:
            #print('###########', found)
            rows += row
            #rows += f'{found[0]}\n'
           
        else:
            rows += row

    return rows


def remove_comments(file):
    chars = file.read().decode('UTF-8')

    #while any(pattern in chars for pattern in ['/*', '//', '*/']):
    chars = re.sub(r'(?<=[^/])/\*.*?\*/', '', chars)
    #chars = re.sub(r'.*?(?=/{2}).*?(?=\n)', '', chars)

    #source_code = remove_multi_line_comments(source_code)
    #source_code = remove_one_line_comments(source_code)

    print(chars)
    

def compile_material(material_data):
    techniques, vertex_attributes, shader_modules = [
        material_data[k] for k in ('techniques', 'vertexAttributes', 'shaderModules')
    ]
    
    for technique_index, technique in enumerate(techniques):
        vertex_attribute_layout = shader_vertex_attribute_layout(vertex_attributes, technique)

        inputs = shader_inputs(vertex_attribute_layout)

        header = shader_header(inputs)

        for shader_module_index in technique['shadersBundle']:
            shader_module = shader_modules[shader_module_index]

            name, stage = shader_module['name'], shader_module['stage']

            source_code = ''

            technique_lines = [ ]

            with open(os.path.join(shaders.source_path, f'{name}.glsl'), 'rb') as file:
                remove_comments(file)
            break

            with open(os.path.join(shaders.source_path, f'{name}.glsl'), 'rb') as file:
                opening_braces = 0
                closing_braces = 0

                rows = file.read().decode('UTF-8')

                while True:
                    mo = re.search(r'(.*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)(.*)\n', rows)

                    if not mo:
                        break

                    opening_braces = len(re.findall(r'\{', mo.group(3)))
                    closing_braces = len(re.findall(r'\}', mo.group(3)))

                    if opening_braces * closing_braces != 0 and opening_braces == closing_braces:
                        print(r'^^^^^^^^^^^^')

                    print('############')
                    print(mo.group(1), mo.group(3), mo.span(), mo.group(2))

                    if int(mo.group(2)) == technique_index:
                        # rows = re.sub(
                        #    r'^(.*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\(technique_index\)(.*)',
                        #    lambda mo : f'{mo.group(1)}void technique{technique_index}(){mo.group(2)}',
                        #    rows
                        # )
                        # print(rows)
                        print(f'technique_index {technique_index}')

                    else:
                        xrows = rows[mo.span()[0]:]
                        print(f'@@@@@@@@@@@@@@@{xrows}')

                        while True:
                            mo2 = re.search(r'\n', xrows)

                            if not mo2:
                                break
                            
                            print(f'\t|{xrows[:mo2.span()[1]]}')

                            xrows = xrows[mo2.span()[1]:]
                        # opening_braces = len(re.findall(r'\{', mo.group(3)))
                        # closing_braces = len(re.findall(r'\}', mo.group(3)))
                        # print(f'===========\n{rows}')

                    rows = rows[mo.span()[1]:]

                #for row_index, row in enumerate(file):
                #    mo = re.search(r'(.*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)(.*)', row)

                #    if mo and int(mo.group(2)) != technique_index:
                #        opening_braces = len(re.findall(r'\{', mo.group(3)))
                #        closing_braces = len(re.findall(r'\}', mo.group(3)))

                #        if opening_braces != 0 and opening_braces == closing_braces:
                #            print(mo.group(1), mo.group(3), mo.span(), mo.group(2))
                #            source_code += f'{mo.group(1)} // {mo.group(3)}'

                    #source_code += re.sub(
                    #    r'^(.*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)(.*)',
                    #    lambda mo : f'{mo.group(1)}void technique{mo.group(2)}(){mo.group(3)}',
                    #    row
                    #)

            source_code_stream = f'{header}\n{source_code}'#.encode('UTF-8')

            # print(source_code)
            # print(source_code_stream)

    return

    module_name = shader_module['name']
    shader_module_absolute_path = os.path.join(shaders.source_path, module_name + '.glsl')

    with open(shader_module_absolute_path, 'r') as file:
        for line in file:
            source_code += re.sub(
                r'^(.*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\((\d+)\)(.*)',
                lambda mo : f'{mo.group(1)}void technique{mo.group(2)}(){mo.group(3)}',
                line
            )

    if not source_code:
        return

    # print(source_code)

    for index in re.findall(r'void technique(\d+)\(\)', source_code):
        technique_index = int(index)
        technique = f'technique{technique_index}'

        vertex_attributes_string = ''

        for vertex_attribute_index in techniques[technique_index]['vertexLayout']:
            vertex_attribute = vertex_attributes[vertex_attribute_index]
            semantic, type = [vertex_attribute[k] for k in ('semantic', 'type')]

            layout = shaders.vertex_attributes_locations[semantic]

            vertex_attributes_string += f'layout (location = {layout}) in {type} {semantic};\n'

        shader_header2 = f'{version_string}\n{extensions_string}\n{vertex_attributes_string}\n#line 0'

        full_source_code = f'{shader_header2}\n{source_code}'

        output_path = os.path.join(shaders.source_path, f'{module_name}.{technique}.spv')

        compiler = subprocess.Popen([
            shaders.compiler_path,
            '-e', technique,
            '--source-entrypoint', 'main',
            '-V',
            # '-H',
            '--target-env', 'vulkan1.1',
            f'-I{shaders.include_path}',
            '-o', output_path,
            '--stdin',
            '-S', 'vert'
        ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        output, errors = compiler.communicate(full_source_code.encode('UTF-8'))

        print(output.decode('UTF-8'))

        if compiler.returncode != 0:
            print(errors.decode('UTF-8'))


for root, dirs, material_relative_paths in os.walk(materials.source_path):
    for material_relative_path in material_relative_paths:
        if not material_relative_path.endswith(materials.file_extensions):
            continue

        if material_relative_path != 'color-debug-material.json':
            continue

        print(material_relative_path)

        material_absolute_path = os.path.join(root, material_relative_path)

        with open(material_absolute_path, 'r') as json_file:
            compile_material(json.load(json_file))

            # material_data = json.load(json_file)
            
            # shader_modules = material_data['shaderModules']

            # vertex_stage_modules = [shader_module for shader_module in shader_modules if shader_module['stage'] == 'vertex']

            # for vertex_stage_module in vertex_stage_modules:
            #     compile_material(material_data, vertex_stage_module)
