CC=g++
CC_FLAGS=`pkg-config --cflags glfw3` -DGL_GLEXT_PROTOTYPES
LD_FLAGS=`pkg-config --static --libs glfw3`
src=glsl_playground.cpp
exec=glsl_playground

all:
	$(CC) $(CC_FLAGS) -o $(exec) $(src) $(LD_FLAGS)

clean:
	rm -rf *o $(exec)
