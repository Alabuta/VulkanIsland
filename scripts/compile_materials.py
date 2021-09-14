import os
import re
import sys
import json
import uuid
import argparse
import traceback
import subprocess

from operator import itemgetter, attrgetter
from functools import reduce, partial
from typing import NamedTuple

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pymodules'))

import custom_exceptions as exs
import utils


class GLSLSettings(NamedTuple):
    extensions: dict

class Shaders(NamedTuple):
    compiler_path: str
    file_extensions: tuple
    glsl_settings: GLSLSettings
    vertex_attributes_locations: dict
    vertex_attributes_types: dict
    primitive_input_layouts: dict
    stage2extension: dict
    processed_shaders: dict


shaders=Shaders(
    compiler_path='glslangValidator',
    file_extensions=('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl'),
    glsl_settings=GLSLSettings(
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
    },
    primitive_input_layouts={
        'points': 'points',
        'lines': 'line_strip',
        'lines_adjacency': 'line_strip',
        'triangles': 'triangle_strip',
        'triangles_adjacency': 'triangle_strip'
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

def parse_program_options():
	argparser = argparse.ArgumentParser(description='Material compiler')

	argparser.add_argument('materials', help='list of materials\' names or directories to compile (e. g. \'debug\' or \'color-debug.json\')', nargs='+')
	argparser.add_argument('-s', '--shaders-src-folder', dest='shaders_src_folder', default='../contents/shaders',
							help='shaders source files folder (default is \'../contents/shaders\')', metavar='<shaders-source-folder>')
	argparser.add_argument('-i', '--shaders-include-folder', dest='shaders_include_folder', default='../contents/shaders/include',
							help='shaders include source files folder (default is \'../contents/shaders/include\')', metavar='<shaders-include-folder>')
	argparser.add_argument('-S', '--materials-src-folder', dest='materials_src_folder', default='../contents/materials',
							help='materials source files folder (default is \'../contents/materials\')', metavar='<materials-source-folder>')
	argparser.add_argument('-o', '--output-path', dest='outpath', default='../contents/shaders/bin',
							help='path to directory where to save compiled shaders (default is \'../contents/shaders/bin\')', metavar='<output-path>')
	argparser.add_argument('--verbose', dest='verbose',
							help='verbose console output (default is false)', action='store_true')
	argparser.add_argument('--glsl-version', dest='glsl_version', default=460,
							help='GLSL shader language version (default is 460)', metavar='<glsl-version>')
	argparser.add_argument('--material-file-ext', dest='mat_file_ext', default='.json',
							help='material file extension (default is json)', metavar='<material-file-ext>')

	return vars(argparser.parse_args())


def shader_directives(program_options):
    """
    A function used to get general shader directives

    Parameters
    ----------
    program_options : dict
        Program options
    """

    version_line=f'#version {program_options["glsl_version"]}\n'
    extensions_lines=reduce(lambda s, e: s+f'#extension {e[0]} : {e[1]}\n', shaders.glsl_settings.extensions.items(), '')

    return (version_line, extensions_lines)


def shader_header_files():
    """
    A function used to get common shader header files
    """
    return '#include "vertex/vertex-attributes-unpack.glsl"\n'


def shader_header(program_options, stage, shader_inputs):
    version_line, extensions_lines=shader_directives(program_options)

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


def shader_primitive_input(primitive_topologies, primitive_input):
    input_layout, out_vertices_count=itemgetter('layout', 'outVerticesCount')(primitive_topologies[primitive_input])
    output_layout=shaders.primitive_input_layouts[input_layout]

    return f'layout ({input_layout}) in;\nlayout ({output_layout}, max_vertices = {out_vertices_count}) out;\n'


def compile_vertex_layout_name(vertex_attributes, vertex_layout):
    getter=itemgetter('semantic','type')
    return '|'.join(map(lambda a: ':'.join(getter(a)).lower(), [vertex_attributes[i] for i in vertex_layout]))

def compile_primitives_input_name(primitive_topology):
    return f'{primitive_topology["type"]}'


# TODO:: add another shader input structures
def shader_inputs(material, vertex_layout):
    vertex_attributes=material['vertexAttributes']

    vertex_attributes_lines=shader_vertex_attribute_layout(vertex_attributes, vertex_layout)

    primitive_topologies_lines=''#shader_geometry_primitive_input(primitive_topology)

    vertex_stage_inputs=f'{vertex_attributes_lines}\n'
    tesc_stage_inputs=f'{primitive_topologies_lines}\n'
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

def vertex_shader_inputs(material, vertex_layout):
    vertex_attributes=material['vertexAttributes']
    vertex_attributes_lines=shader_vertex_attribute_layout(vertex_attributes, vertex_layout)

    return {'vertex': f'{vertex_attributes_lines}\n'}

def geometry_shader_inputs(material, primitive_input):
    primitive_topologies=material['primitiveTopologies']
    primitive_topologies_lines=shader_primitive_input(primitive_topologies, primitive_input)

    return {'geometry': f'{primitive_topologies_lines}\n'}

def fragment_shader_inputs(material):
    return {'fragment': '\n'}


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


def sub_attributes_unpacks(source_code, vertex_layout, vertex_attributes):
    for vertex_attribute_index in vertex_layout:
        vertex_attribute=vertex_attributes[vertex_attribute_index]
        semantic, type=itemgetter('semantic', 'type')(vertex_attribute)

        pattern=rf'unpackAttribute[ |\t]*\([ |\t]*?{semantic}[ |\t]*?\)'
        general_name=semantic.split('_')[0].lower()

        source_code=re.sub(pattern, f'unpackAttribute_{general_name}_{type}({semantic})', source_code, 0, re.DOTALL)

    return source_code


def get_shader_source_code(program_options, name):
    path=f'{name}.glsl'

    if path in shaders.processed_shaders:
        return shaders.processed_shaders[path]

    with open(os.path.join(program_options['shaders_src_folder'], path), 'rb') as file:
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

def compile(program_options, name, stage, inputs):
    header=shader_header(program_options, stage, inputs)

    source_code=get_shader_source_code(program_options, name)

    if not source_code:
        print(f'can\'t get shader source code {name}')
        continue

    source_code=remove_inactive_techniques(technique_index, source_code)

    if stage=='vertex':
        source_code=sub_attributes_unpacks(source_code, vertex_layout, vertex_attributes)

    if 'specializationConstants' in shader_bundle:
        constants=get_specialization_constants(shader_bundle['specializationConstants'])
        source_code=f'{constants}\n{source_code}'

    source_code=f'{header}\n{source_code}'

    shader_name=f'{name}.{technique_index}.{vertex_layout_name}'
    hashed_name=str(uuid.uuid5(uuid.NAMESPACE_DNS, shader_name))
    output_path=os.path.join(program_options['shaders_src_folder'], f'{hashed_name}.spv')

    print(f'{name}.{technique_index}.{vertex_layout_name} -> {output_path}')

def compile_material(program_options, material_data):
    techniques, shader_modules=itemgetter('techniques', 'shaderModules')(material_data)
    vertex_attributes, primitive_topologies=itemgetter('vertexAttributes', 'primitiveTopologies')(material_data)
    
    for technique in techniques:
        # vertex_layouts=map(lambda l: [vertex_attributes[i] for i in l], technique['vertexLayouts'])
        # primitive_inputs=[primitive_topologies[i] for i in technique['primitiveInputs']]

        for shader_bundle in technique['shadersBundle']:
            shader_module_index, technique_index=itemgetter('index', 'technique')(shader_bundle)

            shader_module=shader_modules[shader_module_index]

            name, stage=itemgetter('name', 'stage')(shader_module)

            inputs=''

            if stage=='vertex':
                for vertex_layout in technique['vertexLayouts']:
                    inputs=vertex_shader_inputs(material_data, vertex_layout)

            elif stage=='geometry':
                for primitive_input in technique['primitiveInputs']:
                    inputs=geometry_shader_inputs(material_data, primitive_input)

            elif stage=='fragment':
                inputs=fragment_shader_inputs(material_data);

        return

        for vertex_layout in technique['vertexLayouts']:

            vertex_layout_name=compile_vertex_layout_name(vertex_attributes, vertex_layout)
            # primitives_topology_name=compile_primitives_input_name()

            for shader_bundle in technique['shadersBundle']:
                shader_module_index, technique_index=itemgetter('index', 'technique')(shader_bundle)

                shader_module=shader_modules[shader_module_index]

                name, stage=itemgetter('name', 'stage')(shader_module)

                header=shader_header(program_options, stage, inputs)

                source_code=get_shader_source_code(program_options, name)

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
                output_path=os.path.join(program_options['shaders_src_folder'], f'{hashed_name}.spv')

                print(f'{name}.{technique_index}.{vertex_layout_name} -> {output_path}')

                compiler=subprocess.Popen([
                    shaders.compiler_path,
                    '--entry-point', f'technique{technique_index}',
                    '--source-entrypoint', 'main',
                    '-V',
                    # '-H',
                    '--target-env', 'vulkan1.1',
                    f'-I{program_options["shaders_include_folder"]}',
                    '-o', output_path,
                    '--stdin',
                    '-S', shaders.stage2extension[stage]
                ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                output, errors=compiler.communicate(source_code.encode('UTF-8'))

                output=output.decode('UTF-8')[len('stdin'):]
                if len(output) > 2:
                    raise exs.GLSLangValidatorError(name, output)

                if compiler.returncode!=0:
                    raise exs.GLSLangValidatorError(name, errors.decode('UTF-8'))


def main():
    program_options=parse_program_options()

    try:
        for material in program_options['materials']:
            if not os.path.isdir(material) and not os.path.isfile(material):
                material=os.path.join(program_options['materials_src_folder'], material)

            path=os.path.abspath(material)

            if os.path.isdir(path):
                for dirpath, _, filenames in os.walk(path):
                    filenames=filter(lambda n: n.endswith(program_options['mat_file_ext']), filenames)
                    filenames=map(lambda n: os.path.abspath(os.path.join(dirpath, n)), filenames)

                    for filename in filenames:
                        with open(filename, 'r') as json_file:
                            compile_material(program_options, json.load(json_file))
            
            elif os.path.isfile(path) and path.endswith(program_options['mat_file_ext']):
                with open(path, 'r') as json_file:
                    compile_material(program_options, json.load(json_file))
            

    except exs.GLSLangValidatorError as ex:
        shader_name, msg=attrgetter('shader_name', 'msg')(ex)
        utils.err_print_fmt(f'==== Shader compilation error, shader \'{shader_name}\':', msg)
        traceback.print_exc()


if __name__=='__main__': main()
