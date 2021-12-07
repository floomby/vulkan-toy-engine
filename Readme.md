## Multiplayer RTS lua driven engine

This is aiming to be an engine/game which is more or less a homeworld knockoff. It has multiplayer based on an authoritative server with replication to clients. Lua defines properties, behaviors, states, ui and gameplay running in both a synchronized and unsynchronized context on top of a fairly simple prescribed way of doing things. The engine separates rendering, gui, networking and game state updates into separate threads and allows multithreaded lua usage using multiple instances of lua vms (this seems to be the best option) by providing a thread safe api which handles synchronization and resource sharing while trying to minimize contention.

### Development

#### Main Requirements

This section is mostly notes to myself, but I will put it here for posterity or something.

I am well aware I should just use cmake, but I haven't done this yet mostly due to my proclivity to muddle around and both annoy and confuse myself with cmake every time I do anything with it.

I doubt a windows build will work due to some fcntl stuff. (I know how to fix this and will do it.) Other than that, and possibly boost stacktrace, which can be ifdefed out, I think getting it to build against msys2 should work if a brief googling about if the glfw package and vulkan compatibility is to be believed.

 * gcc 10 or newer
 * freetype 2.11.0 or newer - We need sdf rendering, but the version which ships with ubuntu was not this new. I built from their git repo and it worked fine. We don't need harfbuz so no need to worry that on ubuntu this is too old for using in the freetype build and just letting it be disabled is fine.
 * LuaJIT - I also built this from their git repo. I did not use the ubuntu version as I read on luajit's website that this was inadvisable.
 * shaderc - We don't need to link to anything in this, but we do need their glsl compiler. Another one may work, but I am not sure as I use some features I know are not in vanilla glsl. I built this from their github repo and it was super easy.
 * The vulkan sdk - I acquired this by adding the lunarg repo and installing with apt. (Technically all you need are the headers, which you can get without the whole sdk, and libvulkan, again which you can get without the whole sdk. Having vkconfig though is very useful and this is in the sdk.)
 * Boost - asio, stacktrace, crc, program options, process at the time of writing. (Boost has been my answer to cross platform stuff)
 * Glm
 * Glfw
 * OpenAL
 * libvorbis

#### Lua binding generator

 * ruby, clang and castxml - This is just part of the tooling to save writing boilerplate. If you don't need to change any of the lua api then you can just ignore this, (or you can mess with the bindings by hand). Castxml might get tangled up with gnu vs clang headers and whatnot depending on your system. I had to change stuff in the rbgccxml gem to get it to work on ubuntu with clang from the package manager. (Before you ask, "Why not just build the whole thing with clang?" I already went quite far down this road and it goes nowhere good.)

#### Other stuff

 * ImageMagick - The makefile has a target that checks textures to make sure they are all ok to use. This uses the ImageMagick command line utility.
