cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gnss_master_thesis)

FILE(GLOB root src/*.c)
FILE(GLOB toolkits src/toolkits/*.c)
FILE(GLOB app src/app/*.c)

target_sources(app PRIVATE
  ${root}
  ${toolkits}
  ${app}
)

# App inc
zephyr_library_include_directories(.)
zephyr_library_include_directories(./src)
zephyr_library_include_directories(./src/toolkits)
zephyr_library_include_directories(./src/app)