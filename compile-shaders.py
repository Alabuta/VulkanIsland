import os
from subprocess import call


compiler_path = 'glslangValidator'

shaders_path =  './contents/shaders'
shaders_include_path =  shaders_path + '/include'

shader_extensions = ('.vert.glsl', '.tesc.glsl', '.tese.glsl', '.geom.glsl', '.frag.glsl', '.comp.glsl')

for root, dirs, files in os.walk(shaders_path):
    if 'include' in dirs:
        dirs.remove('include')

    for file in files:
        if file.endswith(shader_extensions):
            in_path = os.path.join(root, file)
            out_path = in_path.replace('.glsl', '.spv')

            call([compiler_path, '-V', '-I' + shaders_include_path, in_path, '-o', out_path])


#if not os.path.exists(out_path):
#    open(out_path, 'x').close()
