cmake_minimum_required(VERSION 3.0)
project(node-velocypack)

#set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++1y") #curlpp has broken build system
add_subdirectory(velocypack)

add_library(vpack SHARED
    VPackBindings.cpp
)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(vpack PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(vpack PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(vpack ${CMAKE_JS_LIB} velocypack)
