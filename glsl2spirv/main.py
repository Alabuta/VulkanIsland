import sys
import os
from subprocess import call


glslangValidatorPath = 'glslangValidator'

shadersDirectory = "{}/../{}".format(os.path.dirname(os.path.realpath(__file__)), "contents/shaders/")

vertPath = "{}shader.vert".format(shadersDirectory)
fragPath = "{}shader.frag".format(shadersDirectory)

vertSpvPath = "{}vert.spv".format(shadersDirectory)
fragSpvPath = "{}frag.spv".format(shadersDirectory)

if not os.path.exists(vertSpvPath):
    open(vertSpvPath, 'x').close()

if not os.path.exists(fragSpvPath):
    open(fragSpvPath, 'x').close()

call([glslangValidatorPath, "-V", vertPath, "-o", vertSpvPath])
call([glslangValidatorPath, "-V", fragPath, "-o", fragSpvPath])
