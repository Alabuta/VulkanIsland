import os
import re
import json
from subprocess import call
from typing import NamedTuple


class Shaders(NamedTuple):
    compiler_path: str
    source_path: str
    include_path: str
    file_extensions: tuple

shaders = Shaders(
    'glslangValidator',
    './contents/shaders',
    './contents/shaders/include',
    ('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl')
)


def parse_vertex_shader_source(shader_source_path):
    output = {}

    with open(shader_source_path, 'r') as file:
        attributes = []

        for line in file:
            found_attributes = re.findall(r'^[ |\t]*#[ |\t]*pragma[ |\t]+attribute[ |\t]*\((.*)\)[ |\t]*\n$', line)

            if found_attributes:
                found_attribute = re.sub('[\s|\t]*', '', found_attributes[0])
                semantic, type, presence = re.split(',', found_attribute)

                attribute = {
                    'semantic': semantic,
                    'type': type,
                    'optional': presence == 'optional'
                }

                attributes.append(attribute)

        if attributes:
            output['attributes'] = attributes

    output_path = shader_source_path.replace('.glsl', '.json')

    with open(output_path, 'w+') as file:
        print(json.dumps(output, indent = 4))

        file.write(json.dumps(output, indent = 4))


for root, dirs, files in os.walk(shaders.source_path):
    if 'include' in dirs:
        dirs.remove('include')

    for file in files:
        if file.endswith(shaders.file_extensions):
            in_path = os.path.join(root, file)
            out_path = in_path.replace('.glsl', '.spv')

            if file.endswith('.vert.glsl'):
                parse_vertex_shader_source(in_path)

            call([shaders.compiler_path, '-V', '-I' + shaders.include_path, in_path, '-o', out_path])
