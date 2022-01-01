cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

project(equinox)

file(GLOB my_srcs *.cpp)
file(GLOB my_fshaders shaders/*.frag)
file(GLOB my_vshaders shaders/*.vert)
file(GLOB my_cshaders shaders/*.comp)

set(my_shaders my_fshaders my_vshaders my_cshaders)

find_package(glfw3 3.3 REQUIRED)
find_package(Freetype 2.11.0 REQUIRED)
find_package(PNG REQUIRED)
find_package(Brotli REQUIRED)
find_package(VorbisFile REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
find_package(Vulkan REQUIRED)

find_package(Boost COMPONENTS program_options stacktrace_backtrace stacktrace_addr2line REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_library(OPENAL openal)

add_custom_target(
    luajit
    COMMAND make
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lua/LuaJIT"
)

add_custom_target(
    rbgccxml
    COMMAND "gem build rbgccxml.gemspec && gem install --local rbgccxml-1.1.0.gem"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lua/rbgccxml"
)

add_custom_target(
    bindings
    DEPENDS "${CMAKE_SOURCE_DIR}/api.hpp" "${CMAKE_SOURCE_DIR}/api_util.hpp" "${CMAKE_SOURCE_DIR}/lua_binding_gen.rb"
    COMMAND "${CMAKE_SOURCE_DIR}/lua_binding_gen.rb"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(
    shaders
    COMMAND "${CMAKE_SOURCE_DIR}/shaders/compile.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/shaders"
)

add_executable(equinox ${my_srcs} "${CMAKE_SOURCE_DIR}/lua/bindings.cpp")

add_dependencies(equinox luajit bindings shaders)

target_link_libraries(equinox
    ${OPENAL}
    ${Boost_STACKTRACE_BACKTRACE_LIBRARY}
    ${Boost_STACKTRACE_ADDR2LINE_LIBRARY}
    ${FREETYPE_LIBRARIES}
    ${PNG_LIBRARIES}
    ${BROTLI_LIBRARIES}
    ${VorbisFile_LIBRARIES}
    "${CMAKE_SOURCE_DIR}/lua/LuaJIT/src/libluajit.a"
    ${Boost_LIBRARIES}
    ${Vulkan_LIBRARIES}
    glfw
    ${CMAKE_DL_LIBS}
    Threads::Threads
    Brotli::brotlicommon
    Brotli::brotlidec
)

set(CMAKE_CXX_FLAGS "-fcoroutines -pedantic -std=c++2a -Ofast -ggdb -DBOOST_STACKTRACE_USE_BACKTRACE -DDONT_PRINT_MEMORY_LOG ")