import os
import sys
import json
import uuid
import argparse
import traceback
import subprocess

from operator import attrgetter, itemgetter
from shader_module_info import ShaderLanguage, ShaderModuleInfo

from shader_constants import ShaderStage
from material_technique import MaterialTechnique

from glsl_preprocessor import GLSLShaderPreprocessor
from hlsl_preprocessor import HLSLShaderPreprocessor
from abstract_shader_preprocessor import AbstractShaderPreprocessor


sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pymodules'))

import custom_exceptions as exs
from utils import err_print_fmt


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
	argparser.add_argument('--shader-compiler-path', dest='shader_compiler_path', default='glslangValidator',
							help='shader compiler path (default is glslangValidator)', metavar='<shader_compiler_path>')

	return vars(argparser.parse_args())


def get_shader_compialtion_flags(shader_module : ShaderModuleInfo) -> list:
    if shader_module.shader_language==ShaderLanguage.GLSL:
        return ['-V']

    elif shader_module.shader_language==ShaderLanguage.HLSL:
        return ['-V', '-D', '--hlsl-enable-16bit-types']


def compile_shader(program_options : object, shader_module : ShaderModuleInfo, output_path : str, source_code : str):
    compialtion_flags=get_shader_compialtion_flags(shader_module)

    dirpath=os.path.dirname(output_path)
    if not os.path.exists(dirpath):
        os.makedirs(dirpath)

    compiler=subprocess.Popen([
        program_options['shader_compiler_path'],
        '--entry-point', shader_module.entry_point,
        '--source-entrypoint', 'main',
        *compialtion_flags,
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


def get_shader_preprocessor(glsl_preprocessor : GLSLShaderPreprocessor, hlsl_preprocessor : HLSLShaderPreprocessor, shader_module : ShaderModuleInfo) -> AbstractShaderPreprocessor:
    if shader_module.shader_language==ShaderLanguage.GLSL:
        return glsl_preprocessor

    elif shader_module.shader_language==ShaderLanguage.HLSL:
        return hlsl_preprocessor


def compile_material(program_options : object, material_data : object):
    shaders_src_folder, glsl_version=itemgetter('shaders_src_folder', 'glsl_version')(program_options)

    glsl_preprocessor=GLSLShaderPreprocessor(shaders_src_folder, glsl_version)
    hlsl_preprocessor=HLSLShaderPreprocessor(shaders_src_folder)

    for i, _ in enumerate(material_data['techniques']):
        material_tech=MaterialTechnique(material_data, i)
        for shader_module in material_tech.shader_bundle:
            shader_preprocessor=get_shader_preprocessor(glsl_preprocessor, hlsl_preprocessor, shader_module)
            shader_preprocessor.process(shader_module)

            hashed_name=str(uuid.uuid5(uuid.NAMESPACE_DNS, shader_module.target_name))
            output_path=os.path.join(program_options['outpath'], f'{hashed_name}.spv')

            print(f'{shader_module.target_name} -> {output_path}')
            compile_shader(program_options, shader_module, output_path, shader_preprocessor.source_code)


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
        err_print_fmt(f'==== Shader compilation error, shader \'{shader_name}\':', msg)
        traceback.print_exc()


if __name__=='__main__': main()
