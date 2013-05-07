GLFW_INCPATH=-I/usr/include
GLFW_LIBS=-lglfw

GLEW_INCPATH=-I/home/syoyo/work/angelina/opengl/deps/linux/glew-1.9.0/include
GLEW_LIBS=-L/home/syoyo/work/angelina/opengl/deps/linux/glew-1.9.0/lib -lGLEW

all:
	g++ -o aobench_cs $(GLFW_INCPATH) $(GLEW_INCPATH) main.cc $(GLFW_LIBS) $(GLEW_LIBS)
