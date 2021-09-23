#!/bin/bash

echo "Compiling shaders"
../shaderc/build/glslc/glslc shader.vert -o vert.spv
../shaderc/build/glslc/glslc shader.frag -o frag.spv
../shaderc/build/glslc/glslc hud.frag -o hud_frag.spv
../shaderc/build/glslc/glslc hud.vert -o hud_vert.spv