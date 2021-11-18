#!/bin/bash

echo "Compiling shaders"
../shaderc/build/glslc/glslc shader.vert -o vert.spv
../shaderc/build/glslc/glslc shader.frag -o frag.spv
../shaderc/build/glslc/glslc line.frag -o line_frag.spv
../shaderc/build/glslc/glslc line.vert -o line_vert.spv
../shaderc/build/glslc/glslc hud.frag -o hud_frag.spv
../shaderc/build/glslc/glslc hud.vert -o hud_vert.spv
../shaderc/build/glslc/glslc shadow.vert -o shadow_vert.spv
../shaderc/build/glslc/glslc sdf_blit.comp -o sdf_blit.spv
