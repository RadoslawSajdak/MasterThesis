# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

FILE(GLOB root src/*.c)
FILE(GLOB toolkits src/toolkits/*.c)

target_sources(app PRIVATE
  ${root}
  ${toolkits}
)

zephyr_library_include_directories(.)
zephyr_library_include_directories(./src)
zephyr_library_include_directories(./src/toolkits)