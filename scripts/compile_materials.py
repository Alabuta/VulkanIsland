import os
import re
import sys
import json
import uuid
import argparse
import traceback
import subprocess

from operator import itemgetter, attrgetter
from typing import NamedTuple

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pymodules'))

import custom_exceptions as exs
import utils


class GLSLSettings(NamedTuple):
    version: int
    extensions: dict

class Shaders(NamedTuple):
    compiler_path: str
    source_path: str
    include_path: str
    file_extensions: tuple
    glsl_settings: GLSLSettings
    vertex_attributes_locations: dict
    vertex_attributes_types: dict
    stage2extension: dict
    processed_shaders: dict

class Materials(NamedTuple):
    source_path: str
    file_extensions: tuple


shaders=Shaders(
    compiler_path='glslangValidator',
    source_path='./shaders',
    include_path='./shaders/include',
    file_extensions=('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl'),
    glsl_settings=GLSLSettings(
        version=460,
        extensions={
            'GL_ARB_separate_shader_objects': 'enable',
            'GL_EXT_shader_16bit_storage': 'enable',
            'GL_EXT_shader_8bit_storage': 'enable',
            # 'VK_KHR_shader_float16_int8 ': 'enable',
            'GL_EXT_scalar_block_layout': 'enable',
            'GL_GOOGLE_include_directive': 'enable'
        }
    ),
    vertex_attributes_locations={
        'POSITION': 0,
        'NORMAL': 1,
        'TEXCOORD_0': 2,
        'TEXCOORD_1': 3,
        'TANGENT': 4,
        'COLOR_0': 5,
        'JOINTS_0': 6,
        'WEIGHTS_0': 7
    },
    vertex_attributes_types={
        'r8i_norm': 'float',
        'rg8i_norm': 'vec2',
        'rgb8i_norm': 'vec3',
        'rgba8i_norm': 'vec4',

        'r8ui_norm': 'float',
        'rg8ui_norm': 'vec2',
        'rgb8ui_norm': 'vec3',
        'rgba8ui_norm': 'vec4',

        'r16i_norm': 'float',
        'rg16i_norm': 'vec2',
        'rgb16i_norm': 'vec3',
        'rgba16i_norm': 'vec4',

        'r16ui_norm': 'float',
        'rg16ui_norm': 'vec2',
        'rgb16ui_norm': 'vec3',
        'rgba16ui_norm': 'vec4',

        'r8i_scaled': 'float',
        'rg8i_scaled': 'vec2',
        'rgb8i_scaled': 'vec3',
        'rgba8i_scaled': 'vec4',

        'r8ui_scaled': 'float',
        'rg8ui_scaled': 'vec2',
        'rgb8ui_scaled': 'vec3',
        'rgba8ui_scaled': 'vec4',

        'r16i_scaled': 'float',
        'rg16i_scaled': 'vec2',
        'rgb16i_scaled': 'vec3',
        'rgba16i_scaled': 'vec4',

        'r16ui_scaled': 'float',
        'rg16ui_scaled': 'vec2',
        'rgb16ui_scaled': 'vec3',
        'rgba16ui_scaled': 'vec4',

        'r8i': 'int',
        'rg8i': 'ivec2',
        'rgb8i': 'ivec3',
        'rgba8i': 'ivec4',

        'r8ui': 'uint',
        'rg8ui': 'uvec2',
        'rgb8ui': 'uvec3',
        'rgba8ui': 'uvec4',

        'r16i': 'int',
        'rg16i': 'ivec2',
        'rgb16i': 'ivec3',
        'rgba16i': 'ivec4',

        'r16ui': 'uint',
        'rg16ui': 'uvec2',
        'rgb16ui': 'uvec3',
        'rgba16ui': 'uvec4',

        'r32i': 'int',
        'rg32i': 'ivec2',
        'rgb32i': 'ivec3',
        'rgba32i': 'ivec4',

        'r32ui': 'uint',
        'rg32ui': 'uvec2',
        'rgb32ui': 'uvec3',
        'rgba32ui': 'uvec4',

        'r32f': 'float',
        'rg32f': 'vec2',
        'rgb32f': 'vec3',
        'rgba32f': 'vec4',

        'r64f': 'double',
        'rg64f': 'dvec2',
        'rgb64f': 'dvec3',
        'rgba64f': 'dvec4'
    }
    ,
    stage2extension={
        'vertex': 'vert',
        'tesselation_control': 'tesc',
        'tesselation_evaluation': 'tese',
        'geometry': 'geom',
        'fragment': 'frag',
        'compute': 'comp'
    },
    processed_shaders={ }
)

materials=Materials(
    source_path='./materials',
    file_extensions=('.json')
)

def parse_program_options():
	argparser = argparse.ArgumentParser(description='Material compiler')

	argparser.add_argument('materials', help='list of material names to compile (e. g. \'debug\')', nargs='+')
	argparser.add_argument('-s', '--shaders-src-folder', dest='shaders_src_folder', default='../contents/shaders',
							help='shaders source files folder (default is \'../contents/shaders\')', metavar='<shaders-source-folder>')
	argparser.add_argument('-S', '--materials-src-folder', dest='materials_src_folder', default='../contents/materials',
							help='materials source files folder (default is \'../contents/materials\')', metavar='<materials-source-folder>')
	argparser.add_argument('-o', '--output-path', dest='outpath', default='../contents/shaders/bin',
							help='path to directory where to save compiled shaders (default is \'../contents/shaders/bin\')', metavar='<output-path>')
	argparser.add_argument('--verbose', dest='verbose',
							help='verbose console output (default is false)', action='store_true')

	return vars(argparser.parse_args())


def shader_directives():
    version_line=f'#version {shaders.glsl_settings.version}\n'

    extensions_lines=''

    for extension, behavior in shaders.glsl_settings.extensions.items():
        extensions_lines +=  f'#extension {extension} : {behavior}\n'

    return (version_line, extensions_lines)


def shader_header_files():
    return '#include "vertex/vertex-attributes-unpack.glsl"\n'


def shader_header(stage, shader_inputs):
    version_line, extensions_lines=shader_directives()

    header_files=shader_header_files()

    stage_inputs=shader_inputs[stage]

    return f'{version_line}\n{extensions_lines}\n{header_files}\n{stage_inputs}\n#line 0'


def shader_vertex_attribute_layout(vertex_attributes, vertex_layout):
    vertex_attribute_layout=[ ]

    for vertex_attribute_index in vertex_layout:
        vertex_attribute=vertex_attributes[vertex_attribute_index]

        semantic, type=itemgetter('semantic', 'type')(vertex_attribute)

        vertex_attribute_layout.append([ semantic, type ])

    vertex_attributes_lines=''

    for vertex_attribute in vertex_attribute_layout:
        semantic, attribute_type=vertex_attribute
        attribute_type=shaders.vertex_attributes_types[attribute_type]

        location=shaders.vertex_attributes_locations[semantic]

        vertex_attributes_lines +=  f'layout (location = {location}) in {attribute_type} {semantic};\n'

    return vertex_attributes_lines


def compile_vertex_layout_name(vertex_attributes, vertex_layout):
    getter=itemgetter('semantic','type')
    return '|'.join(map(lambda a: ':'.join(getter(a)).lower(), [vertex_attributes[i] for i in vertex_layout]))


# TODO:: add another shader input structures
def shader_inputs(material, vertex_layout):
    vertex_attributes=material['vertexAttributes']

    vertex_attributes_lines=shader_vertex_attribute_layout(vertex_attributes, vertex_layout)

    vertex_stage_inputs=f'{vertex_attributes_lines}\n'
    tesc_stage_inputs=f'\n'
    tese_stage_inputs=f'\n'
    geometry_stage_inputs=f'\n'
    fragment_stage_inputs=f'\n'
    compute_stage_inputs=f'\n'

    inputs={
        'vertex': vertex_stage_inputs,
        'tesselation_control': tesc_stage_inputs,
        'tesselation_evaluation': tese_stage_inputs,
        'geometry': geometry_stage_inputs,
        'fragment': fragment_stage_inputs,
        'compute': compute_stage_inputs
    }

    return inputs


def remove_comments(source_code):
    pattern=r'(?://[^\n]*|/\*(?:(?!\*/).)*\*/)'

    substrs=re.findall(pattern, source_code, re.DOTALL)
    
    for substr in substrs:
        source_code=source_code.replace(substr, '\n' * substr.count('\n'))

    return source_code


def sub_techniques(source_code):
    pattern=r'([^\n]*)[ |\t]*#[ |\t]*pragma[ |\t]+technique[ |\t]*\([ |\t]*?(\d+)[ |\t]*?\)([^\n]*)'

    return re.sub(pattern, r'\1void technique\2()\3', source_code, 0, re.DOTALL)
    

def remove_inactive_techniques(technique_index, source_code):
    pattern=r'void technique[^I]\(\).*?{(.*?)}'
    pattern=pattern.replace('I', str(technique_index))

    while True:
        match=re.search(pattern, source_code, re.DOTALL)

        if not match:
            break
    
        substr=match.group(0)

        opening_brackets=substr.count('{')
        closing_brackets=substr.count('}')

        if opening_brackets == closing_brackets:
            source_code=source_code.replace(substr, '\n' * substr.count('\n'))

        else:
            assert opening_brackets > closing_brackets, 'opening brackets count less than closing ones'

            start, end=match.span()

            while True:
                substr=source_code[end:]

                match=re.search(r'.*?}', substr, re.DOTALL)

                if not match:
                    break

                end += match.span()[1]

                opening_brackets += match.group(0).count('{')
                closing_brackets += match.group(0).count('}')

                if opening_brackets == closing_brackets:
                    substr=source_code[start:end]
                    source_code=source_code.replace(substr, '\n' * substr.count('\n'))

                    break

    return source_code


def sub_attributes_unpacks(source_code, vertex_layout ,vertex_attributes):
    for vertex_attribute_index in vertex_layout:
        vertex_attribute=vertex_attributes[vertex_attribute_index]
        semantic, type=itemgetter('semantic', 'type')(vertex_attribute)

        pattern=rf'unpackAttribute[ |\t]*\([ |\t]*?{semantic}[ |\t]*?\)'
        general_name=semantic.split('_')[0].lower()

        source_code=re.sub(pattern, f'unpackAttribute_{general_name}_{type}({semantic})', source_code, 0, re.DOTALL)

    return source_code


def get_shader_source_code(name):
    path=f'{name}.glsl'

    if path in shaders.processed_shaders:
        return shaders.processed_shaders[path]

    with open(os.path.join(shaders.source_path, path), 'rb') as file:
        source_code=file.read().decode('UTF-8')

        source_code=remove_comments(source_code)
        source_code=sub_techniques(source_code)

        shaders.processed_shaders[path]=source_code

        return source_code

    return None


def get_specialization_constants(specialization_constants):
    constants=''

    for index, specialization_constant in enumerate(specialization_constants):
        name, value, type=itemgetter('name', 'value', 'type')(specialization_constant)

        constants += f'layout(constant_id = {index}) const {type} {name} = {type}({value});\n'

    return constants


def compile_material(material_data):
    techniques, shader_modules=itemgetter('techniques', 'shaderModules')(material_data)
    
    for technique in techniques:
        for vertex_layout in technique['vertexLayouts']:
            inputs=shader_inputs(material_data, vertex_layout)

            vertex_attributes=material_data['vertexAttributes']

            vertex_layout_name=compile_vertex_layout_name(vertex_attributes, vertex_layout)

            for shader_bundle in technique['shadersBundle']:
                shader_module_index, technique_index=itemgetter('index', 'technique')(shader_bundle)

                shader_module=shader_modules[shader_module_index]

                name, stage=itemgetter('name', 'stage')(shader_module)

                header=shader_header(stage, inputs)

                source_code=get_shader_source_code(name)

                if not source_code:
                    print(f'can\'t get shader source code {name}')
                    continue

                source_code=remove_inactive_techniques(technique_index, source_code)

                source_code=sub_attributes_unpacks(source_code, vertex_layout, vertex_attributes)

                if 'specializationConstants' in shader_bundle:
                    constants=get_specialization_constants(shader_bundle['specializationConstants'])
                    source_code=f'{constants}\n{source_code}'

                source_code=f'{header}\n{source_code}'

                hashed_name=str(uuid.uuid5(uuid.NAMESPACE_DNS, f'{name}.{technique_index}.{vertex_layout_name}'))
                output_path=os.path.join(shaders.source_path, f'{hashed_name}.spv')

                print(f'{name}.{technique_index}.{vertex_layout_name} -> {output_path}')

                compiler=subprocess.Popen([
                    shaders.compiler_path,
                    '--entry-point', f'technique{technique_index}',
                    '--source-entrypoint', 'main',
                    '-V',
                    # '-H',
                    '--target-env', 'vulkan1.1',
                    f'-I{shaders.include_path}',
                    '-o', output_path,
                    '--stdin',
                    '-S', shaders.stage2extension[stage]
                ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                output, errors=compiler.communicate(source_code.encode('UTF-8'))

                output=output.decode('UTF-8')[len('stdin'):]
                if len(output) > 2:
                    print(output)

                if compiler.returncode!=0:
                    print(errors.decode('UTF-8'), file=sys.stderr)


def main():
    program_options=parse_program_options()

    try:
        # utils.print_fmt('=========', program_options['materials'])
        print('=========', program_options['materials'])

    except exs.GLSLangValidatorError as ex:
        shader_name, msg=attrgetter('shader_name', 'msg')(ex)
        utils.err_print_fmt(f'==== TexturePacker error, shader \'{shader_name}\':', msg)
        traceback.print_exc()



    # for root, dirs, material_relative_paths in os.walk(materials.source_path):
    #     for material_relative_path in material_relative_paths:
    #         if not material_relative_path.endswith(materials.file_extensions):
    #             continue

    #         material_absolute_path=os.path.join(root, material_relative_path)

    #         with open(material_absolute_path, 'r') as json_file:
    #             compile_material(json.load(json_file))


if __name__=='__main__': main()
