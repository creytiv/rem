#
# CMakeLists.txt
#
# Copyright (C) 2010 - 2022 Alfred E. Heggestad
#

cmake_minimum_required(VERSION 3.12)

project(rem VERSION 1.0.0)
include_directories(
  include
  /usr/local/include
  /usr/local/include/re
)
link_directories(/usr/local/lib)
add_compile_definitions(VERSION="${PROJECT_VERSION}")

add_library(${PROJECT_NAME}
  src/aac/aac.c
  src/au/fmt.c
  src/aubuf/aubuf.c
  src/auconv/auconv.c
  src/aufile/aufile.c
  src/aufile/wave.c
  src/auframe/auframe.c
  src/aulevel/aulevel.c
  src/aumix/aumix.c
  src/auresamp/resamp.c
  src/autone/tone.c
  src/avc/config.c
  src/dtmf/dec.c
  src/fir/fir.c
  src/g711/g711.c
  src/goertzel/goertzel.c
  src/h264/getbit.c
  src/h264/nal.c
  src/h264/sps.c
  src/vid/draw.c
  src/vid/fmt.c
  src/vid/frame.c
  src/vidconv/vconv.c
  src/vidmix/vidmix.c
)

target_link_libraries(${PROJECT_NAME} -lre -lpthread)
