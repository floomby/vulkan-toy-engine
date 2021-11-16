## Development

### Main Requirements

I should just use cmake or something, but I didn't do that yet.

 * gcc 10 or newer
 * freetype 2.11.0 or newer - We need sdf rendering, but the version which ships with ubuntu was not this new. I built from their git repo and it worked fine. We don't need harfbuz so no need to worry that on ubuntu this is too old for using in the freetype build and just letting it be disabled is fine.
 * LuaJIT - I also built this from their git repo. I did not use the ubuntu version as I read on luajit's website that this was inadvisable.
 * shaderc - We don't need to link to anything in this, but we do need their glsl compiler. Another one may work, but I am not sure as I use some features I know are not in vanilla glsl. I built this from their github repo.
 * The vulkan sdk - I aquired this by adding the lunarg repo and installing with apt. (Technically all you need are the headers, which you can get without the whole sdk, and libvulkan, again which you can get without the whole sdk. Having vkconfig though is very usefull and this is in the sdk.)

### Lua binding generator

 * ruby, clang and castxml - This is just part of the tooling to save writing boilerplate. If you don't need to change any of the lua api then you can just ignore this, (or you can mess with the bindings by hand).

### Other stuff

 * ImageMagick - The makefile has a target that checks textures to make sure they are all ok to use. This uses the ImageMagick command line utility.