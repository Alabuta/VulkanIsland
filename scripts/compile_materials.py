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

from shader_constants import ShaderStage
from material_technique import MaterialTechnique
from glsl_preprocessor import GLSLShaderPreprocessor


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


def compile_shader(program_options, shader_module, output_path, source_code):
    # api_specific_flags=(
    #     'glsl' : ['-V'],
    #     'hlsl' : ['-D', '--hlsl-enable-16bit-types']
    # )
    compiler=subprocess.Popen([
        shaders.compiler_path,
        '--entry-point', shader_module.entry_point,
        '--source-entrypoint', 'main',
        '-V',
        # '-H',
        '--target-env', 'spirv1.3',
        f'-I{program_options["shaders_include_folder"]}',
        '-o', output_path,
        '--stdin',
        '-S', ShaderStage.to_str(shader_module.stage)
    ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    output, errors=compiler.communicate(source_code.encode('UTF-8'))

    output=output.decode('UTF-8')[len('stdin'):]
    if len(output) > 2:
        raise exs.GLSLangValidatorError(shader_module.target_name, output)

    if compiler.returncode!=0:
        raise exs.GLSLangValidatorError(shader_module.target_name, errors.decode('UTF-8'))



def compile_material(program_options, material_data):
    glsl_preprocessor=GLSLShaderPreprocessor(program_options, program_options["glsl_version"], shaders.glsl_settings.extensions.items())
    
    for i, technique in enumerate(material_data['techniques']):
        material_tech=MaterialTechnique(material_data, i)
        for shader_module in material_tech.shader_bundle:
            glsl_preprocessor.process(shader_module)

            hashed_name=str(uuid.uuid5(uuid.NAMESPACE_DNS, shader_module.target_name))
            output_path=os.path.join(program_options['shaders_src_folder'], f'{hashed_name}.spv')

            print(f'{shader_module.target_name} -> {output_path}')
            compile_shader(program_options, shader_module, output_path, glsl_preprocessor.source_code)


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
