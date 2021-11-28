CFLAGS = -std=c++20 -Ofast -ggdb -DBOOST_STACKTRACE_USE_BACKTRACE -Ilibs/freetype/freetype/include -Ilibs/freetype/freetype/build/include -MMD -fcoroutines -pedantic -DDONT_PRINT_MEMORY_LOG
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lboost_program_options -lbacktrace $(shell pwd)/libs/freetype/freetype/build/libfreetype.a -lz -lpng16 -lbrotlidec -lbrotlicommon $(shell pwd)/lua/LuaJIT/src/libluajit.a

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
GLSL = $(wildcard shaders/*.frag) $(wildcard shaders/*.vert) $(wildcard shaders/*.comp)

engine: bindings.o $(OBJ)
	g++ $^ $(CFLAGS) -o result $(LDFLAGS)

bindings.o: lua/bindings.cpp
	g++ $(CFLAGS) -o $@ -c $<

%.o: %.cpp
	g++ $(CFLAGS) -o $@ -c $<

-include $(DEP)

shaders: $(GLSL) shaders/render_modes.h 
	cd shaders && ./compile.sh

lua/bindings.cpp: api.hpp lua_binding_gen.rb
	ruby lua_binding_gen.rb > lua/bindings.cpp


.PHONY: clean check_formats

clean:
	rm -f result $(OBJ) $(DEP) lua/bindings.cpp

# needs ImageMagik
check_formats: $(wildcard textures/*.png)
	$(foreach file, $^,  echo -n $(file) ":"; identify -format '%[channels]' $(file); echo '';)