cmake_minimum_required(VERSION 3.16.4)

project(TheInstrumenter LANGUAGES CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
#set(BOOST_LIB_PREFIX "lib")
#set(Boost_ARCHITECTURE "-x32")
#set(Boost_COMPILER "mgw81")
#set(Boost_DEBUG 1)
find_package(Boost 1.72.0 REQUIRED COMPONENTS
  filesystem
  system
)

if(Boost_FOUND)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

    file(GLOB SOURCES
      3rdparty/tinyxml2/tinyxml2.cpp
      3rdparty/tinyxml2/tinyxml2.h

      src/include/txlwrapper.h
      src/txlwrapper.cpp

      src/include/common.h

      src/main.cpp
    )

    add_executable(TheInstrumenter ${SOURCES})

    target_link_libraries(TheInstrumenter
      PRIVATE
        $<$<BOOL:${WIN32}>:ws2_32>
        ${Boost_LIBRARIES}
    )
endif()
