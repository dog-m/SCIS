cmake_minimum_required(VERSION 3.16.4)

# Source Code Instrumentation System
project(SCIS LANGUAGES CXX)

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
set(Boost_DEBUG 1)
find_package(Boost 1.72.0 REQUIRED COMPONENTS
  filesystem
  system
  thread
)

if(Boost_FOUND)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

    include_directories(3rdparty src/include)

    file(GLOB SOURCES
      3rdparty/tinyxml2/tinyxml2.cpp
      3rdparty/tinyxml2/tinyxml2.h

      src/txl/grammar.h
      src/txl/grammar.cpp
      src/txl/grammar_parser.h
      src/txl/grammar_parser.cpp

      src/txl/interpreter.h
      src/txl/interpreter.cpp

      src/txl/wrapper.h
      src/txl/wrapper.cpp

      src/scis/annotation.h
      src/scis/annotation.cpp
      src/scis/annotation_parser.h
      src/scis/annotation_parser.cpp

      src/scis/fragment.h
      src/scis/fragment.cpp
      src/scis/fragment_parser.h
      src/scis/fragment_parser.cpp

      src/scis/ruleset.h
      src/scis/ruleset.cpp
      src/scis/ruleset_parser.h
      src/scis/ruleset_parser.cpp

      src/scis/txl_generator.h
      src/scis/txl_generator.cpp

      src/include/arguments.h
      src/include/common.h
      src/include/logging.h

      src/xml_parser_utils.h
      src/xml_parser_utils.cpp

      src/main.cpp
    )

    add_executable(SCIS ${SOURCES})

    target_link_libraries(SCIS
      PRIVATE
        $<$<BOOL:${WIN32}>:ws2_32>
        ${Boost_LIBRARIES}
    )
endif()
