#!/bin/bash

echo "Compiling shaders"
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc line.frag -o line_frag.spv
glslc line.vert -o line_vert.spv
glslc hud.frag -o hud_frag.spv
glslc hud.vert -o hud_vert.spv
glslc shadow.vert -o shadow_vert.spv
glslc sdf_blit.comp -o sdf_blit.spv
