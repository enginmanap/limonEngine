cmake_minimum_required(VERSION 3.3)
project(UberGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp src/SDL2Helper.cpp src/SDL2Helper.h src/GLHelper.cpp src/GLHelper.h)
add_executable(UberGame ${SOURCE_FILES})
set(SDL2_FOUND 1)
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})

find_package(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})

if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

TARGET_LINK_LIBRARIES(UberGame ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})

configure_file(bin/fragment.shader fragment.shader COPYONLY)
configure_file(bin/vertex.shader vertex.shader COPYONLY)
configure_file(bin/glew32.dll glew32.dll COPYONLY)


