cmake_minimum_required(VERSION 3.2)
project(node-velocypack)

set(CMAKE_CXX_STANDARD 14)

# disable to ease debugging (avoids fiddling with LD_PRELOAD)
# set(CMAKE_CXX_FLAGS_DEBUG "-lasan -fsanitize=address -fno-omit-frame-pointer")

option(BuildVelocyPackExamples "Build examples" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC") #curlpp has broken build system
add_subdirectory(velocypack)

add_library(node-velocypack SHARED
    VPackBindings.cpp
)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(node-velocypack PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(node-velocypack PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(node-velocypack ${CMAKE_JS_LIB} velocypack)
