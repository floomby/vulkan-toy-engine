#!/bin/bash

echo "Compiling shaders"
../shaderc/build/glslc/glslc shader.vert -o vert.spv
../shaderc/build/glslc/glslc shader.frag -o frag.spv