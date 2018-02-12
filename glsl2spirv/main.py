import sys
import os
from subprocess import call

vulkanSDKPath = os.environ.get('VULKAN_SDK')

if vulkanSDKPath is None:
    print("Failed to find 'VULKAN_SDK' enviroment variable.")
    sys.exit()
    
glslangValidatorPath = "{}\\Bin\\{}".format(vulkanSDKPath, 'glslangValidator.exe')

shadersDirectory = "{}\\..\\{}".format(os.getcwd(), "VulkanIsland\\shaders\\");

call([glslangValidatorPath, "-V", "-o", "{}vert.spv".format(shadersDirectory), "{}shader.vert".format(shadersDirectory)], shell=True)
call([glslangValidatorPath, "-V", "-o", "{}frag.spv".format(shadersDirectory), "{}shader.frag".format(shadersDirectory)], shell=True)
