CFLAGS = -std=c++20 -O2 -ggdb -DBOOST_STACKTRACE_USE_BACKTRACE -Ilibs/freetype/freetype/include -Ilibs/freetype/freetype/build/include -MMD
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lboost_program_options -lbacktrace -lyaml-cpp $(shell pwd)/libs/freetype/freetype/build/libfreetype.a -lz -lpng16 -lbrotlidec -lbrotlicommon

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
GLSL = $(wildcard shaders/*.frag) $(wildcard shaders/*.vert)

# engine: entry.cpp engine.cpp engine.hpp entity.cpp entity.hpp instance.hpp instance.cpp gui.cpp gui.hpp
# g++ $(CFLAGS) -o result entry.cpp engine.cpp entity.cpp instance.cpp gui.cpp $(LDFLAGS)
engine: $(OBJ)
	g++ $^ $(CFLAGS) -o result $(LDFLAGS)

%.o: %.cpp
	g++ $(CFLAGS) -o $@ -c $<

-include $(DEP)

shaders: $(GLSL) shaders/render_modes.h 
	cd shaders && ./compile.sh

.PHONY: clean check_formats

clean:
	rm -f result $(OBJ) $(DEP)

# needs ImageMagik
check_formats: $(wildcard textures/*.png)
	$(foreach file, $^,  echo -n $(file) ":"; identify -format '%[channels]' $(file); echo '';)

# @identify -format '%[channels]' $^