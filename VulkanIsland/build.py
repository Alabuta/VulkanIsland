import sys
import os
import shutil

# from conans.client. import 

buildFolder = "./build"

if (os.path.isdir(buildFolder)):
    shutil.rmtree(buildFolder)

os.makedirs(buildFolder)
os.chdir(buildFolder)

print(os.getcwd())

# main(["install", "-s", "build_type=Debug"])
# conan install .. -s build_type=Debug
# cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
# ninja -j16
