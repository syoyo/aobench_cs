# OpenGL compute shader version of aobench

![aobench_cs_nv](https://github.com/syoyo/aobench_cs/blob/master/aobench_cs.png?raw=true)
![aobench_cs_radeon](https://github.com/syoyo/aobench_cs/blob/master/aobench_glcompute_7970.png?raw=true)

## Requirements

You'll be interested in Erwin's fork https://github.com/erwincoumans/aobench_cs for easy to build/run on Windows/Mac/Linux environment.

### Hardware

OpenGL compute shader capable hardware and driver. Currently,

* NVIDIA GeForce(310 or later driver)
* AMD Radeon(Catalyst 13.4 or later driver)

is said to be able to run compute shader.

### Software

* GLFW http://www.glfw.org/
* GLEW http://glew.sourceforge.net/


## How to build and run

Edit Makefile, then

    $ make
    $ ./aobench_cs

## Licnese

2-clause BSD.
