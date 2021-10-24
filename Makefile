CFLAGS = -std=c++20 -O2 -ggdb -DBOOST_STACKTRACE_USE_BACKTRACE -Ilibs/freetype/freetype/include -Ilibs/freetype/freetype/build/include
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lboost_program_options -lbacktrace -lyaml-cpp $(shell pwd)/libs/freetype/freetype/build/libfreetype.a -lz -lpng16 -lbrotlidec -lbrotlicommon

engine: entry.cpp engine.cpp engine.hpp entity.cpp entity.hpp instance.hpp instance.cpp gui.cpp gui.hpp
	g++ $(CFLAGS) -o result entry.cpp engine.cpp entity.cpp instance.cpp gui.cpp $(LDFLAGS)

shaders: shaders/shader.frag shaders/shader.vert shaders/hud.frag shaders/hud.vert
	cd shaders && ./compile.sh

.PHONY: clean

clean:
	rm -f result
