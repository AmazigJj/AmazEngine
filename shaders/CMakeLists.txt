cmake_minimum_required (VERSION 3.8)

add_shaders(Shaders "default_lit.frag" "textured_lit.frag" "tri_mesh.vert" "specular_map.frag" "shadow.vert" "shadow.frag" "fullscreen.vert" "tonemap.frag" "cullLights.comp" "depthReduce.comp" "clusterLightCull.comp")