#!/usr/bin/env bash

rm -rf ./cmake
mkdir ./cmake
cd ./cmake

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ ..
make -j`nproc`