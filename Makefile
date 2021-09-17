CFLAGS = -std=c++20 -O2 -ggdb -DBOOST_STACKTRACE_USE_BACKTRACE
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lboost_program_options -lbacktrace

shaders: shaders/shader.frag shaders/shader.vert
	cd shaders && ./compile.sh

.PHONY: clean

engine: entry.cpp engine.cpp engine.hpp entity.cpp entity.hpp instance.hpp instance.cpp
	g++ $(CFLAGS) -o result entry.cpp engine.cpp entity.cpp instance.cpp $(LDFLAGS)

clean:
	rm -f result
