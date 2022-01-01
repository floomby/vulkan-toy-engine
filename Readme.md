## Multiplayer RTS lua driven engine

This is aiming to be an engine/game which is more or less a homeworld knockoff. It has multiplayer based on an authoritative server with replication to clients. Lua defines properties, behaviors, states, ui and gameplay running in both a synchronized and unsynchronized context on top of a fairly simple prescribed way of doing things. The engine separates rendering, gui, networking and game state updates into separate threads and allows multithreaded lua usage using multiple instances of lua vms (this seems to be the best option) by providing a thread safe api which handles synchronization and resource sharing while trying to minimize contention.

### Development

#### Build Requirements

This section is mostly notes to myself, but I will put it here for posterity or something.

I doubt a windows build will work due to some fcntl stuff. (I know how to fix this and will do it.) Other than that, and possibly boost stacktrace, which can be ifdefed out, I think getting it to build against msys2 should work if a brief googling about if the glfw package and vulkan compatibility is to be believed.

 * gcc 10
 * freetype (2.11.0 minimum, cmake knows whats up)
 * libpng
 * LuaJIT - This is submoduled in the repo and should build no problems
 * The vulkan sdk needs to be installed and properly in the path
 * Boost - asio, stacktrace, crc, program options, process, random, lockfree (maybe I am missing one)
 * Glm
 * Glfw
 * OpenAL
 * libvorbis
 * ruby - There is a code generator thing I wrote that uses this
 * castxml - used by the code generator
 * clang (needs to supporet c++20) - again used by the code generator

#### Building

If all the dependencies

```
mkdir build
cd build
cmake ..
make
```

There is no install target. Also the cmake file is pretty bad. It should make a working makefile (at least it does for me), but it does not do a very amazing job checking all the dependencies are there.
