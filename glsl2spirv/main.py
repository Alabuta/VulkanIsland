import sys
import os
from subprocess import call

vulkanSDKPath = os.environ.get('VULKAN_SDK')

if vulkanSDKPath is None:
    print("Failed to find 'VULKAN_SDK' enviroment variable.")
    sys.exit()

glslangValidatorPath = 'glslangValidator'

shadersDirectory = "{}/../{}".format(os.path.dirname(os.path.realpath(__file__)), "shaders/")

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
