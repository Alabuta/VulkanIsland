import os
import re
import json
from subprocess import call


compiler_path = 'glslangValidator'

shaders_path =  './contents/shaders'
shaders_include_path =  shaders_path + '/include'

shader_extensions = ('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl')


def parse_shader_source(shader_source_path):
    file = open(shader_source_path, 'r+')

    for line in file:
        found_attributes = re.findall(r'^[ |\t]*#[ |\t]*pragma[ |\t]+attribute[ |\t]*\((.*)\)[ |\t]*\n$', line)

        if found_attributes:
            found_attribute = re.sub('[\s|\t]*', '', found_attributes[0])
            semantic, type, optional = re.split(',', found_attribute)

            attribute = {
                'semantic': semantic,
                'type': type,
                'optional': optional
            }

            out_path = shader_source_path.replace('.glsl', '.json')

            print(out_path, attribute)

            


for root, dirs, files in os.walk(shaders_path):
    if 'include' in dirs:
        dirs.remove('include')

    for file in files:
        if file.endswith(shader_extensions):
            in_path = os.path.join(root, file)
            out_path = in_path.replace('.glsl', '.spv')

            parse_shader_source(in_path)

            call([compiler_path, '-V', '-I' + shaders_include_path, in_path, '-o', out_path])


#if not os.path.exists(out_path):
#    open(out_path, 'x').close()
