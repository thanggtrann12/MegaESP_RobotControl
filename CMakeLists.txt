cmake_minimum_required(VERSION 3.16)
project(MegaESP_RobotControl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HW_VARIANT "s3_devkit" CACHE STRING "Hardware variant (folder name under cfg/)")
option(BUILD_UTEST "Build and run unit tests on the host (uses utest/mock instead of the Arduino core)" OFF)

# Firmware itself is built with PlatformIO (pio run -e hub). This file exists for host unit tests.
if(NOT BUILD_UTEST)
  message(STATUS "Nothing to build: use PlatformIO for firmware, or -DBUILD_UTEST=ON for host tests.")
  return()
endif()

include_directories(${CMAKE_SOURCE_DIR}/utest/mock)

# Every component has its own CMakeLists.txt and creates a target named after its folder.
file(GLOB COMPONENT_DIRS LIST_DIRECTORIES true
     RELATIVE ${CMAKE_SOURCE_DIR}/components ${CMAKE_SOURCE_DIR}/components/*)
foreach(c ${COMPONENT_DIRS})
  if(EXISTS ${CMAKE_SOURCE_DIR}/components/${c}/CMakeLists.txt)
    add_subdirectory(components/${c})
  endif()
endforeach()

enable_testing()
add_subdirectory(utest)
