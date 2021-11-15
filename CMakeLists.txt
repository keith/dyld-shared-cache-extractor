project(dyld-shared-cache-extractor LANGUAGES C)
cmake_minimum_required(VERSION 3.21)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "Default build type: RelWithDebInfo" FORCE)
endif()

add_executable(dyld-shared-cache-extractor dyld-shared-cache-extractor.c)
target_compile_options(dyld-shared-cache-extractor PRIVATE -g -Weverything)
install(TARGETS dyld-shared-cache-extractor)
