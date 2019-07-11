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


class Materials(NamedTuple):
    source_path: str

materials = Materials('./contents/materials')


for root, dirs, relative_paths in os.walk(materials.source_path):
    for relative_path in relative_paths:
        absolute_path = os.path.join(root, relative_path)

        with open(absolute_path, 'r') as json_file:
            data = json.load(json_file)

            print(data)
