cmake_minimum_required(VERSION 3.5)

project(TheInstrumenter LANGUAGES CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_subdirectory(3rdparty/tinyxml2)

file(GLOB SOURCES
  3rdparty/tinyxml2/tinyxml2.cpp
  3rdparty/tinyxml2/tinyxml2.h

  main.cpp
)

add_executable(TheInstrumenter ${SOURCES})
