#!/bin/sh

# build
ninja -C build3

# run cd build3
./build3/bin/mmm.sdl ./build3/examples/wave

