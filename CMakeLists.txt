#
# CMakeLists.txt
#
# Copyright (C) 2010 - 2022 Alfred E. Heggestad
#

##############################################################################
#
# Versioning
#

cmake_minimum_required(VERSION 3.7)

project(rem VERSION 2.0.0 LANGUAGES C)

##############################################################################
#
# Module Includes
#
include(CheckIncludeFile)

##############################################################################
#
# Compile options
#

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

if(NOT MSVC)
  add_compile_options(
    -Waggregate-return
    -Wall
    -Wbad-function-cast
    -Wcast-align
    -Wdeclaration-after-statement
    -Wextra
    -Wmissing-declarations
    -Wmissing-prototypes
    -Wnested-externs
    -Wold-style-definition
    -Wshadow
    -Wstrict-prototypes
    -Wvla
  )
endif()

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
  add_definitions(-DHAVE_UNISTD_H)
endif()

include_directories(
  include
  ../re/include
  /usr/local/include
  /usr/local/include/re
)
link_directories(/usr/local/lib)
add_definitions(-DVERSION="${PROJECT_VERSION}")

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
  src/h265/nal.c
  src/vid/draw.c
  src/vid/fmt.c
  src/vid/frame.c
  src/vidconv/vconv.c
  src/vidmix/vidmix.c
)

##############################################################################
#
# Main target object
#

target_link_libraries(${PROJECT_NAME} -lre -lpthread)
