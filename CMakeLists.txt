cmake_minimum_required(VERSION 3.3)
project(UberGame)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp src/SDL2Helper.cpp src/SDL2Helper.h src/GLHelper.cpp src/GLHelper.h src/Model.cpp src/Model.h src/World.cpp src/World.h src/InputHandler.cpp src/InputHandler.h src/Camera.cpp src/Camera.h src/SkyBox.cpp src/SkyBox.h src/Texture.cpp src/Texture.h src/CubeMap.cpp src/CubeMap.h src/GLSLProgram.cpp src/GLSLProgram.h src/Renderable.cpp src/Renderable.h)
add_executable(UberGame ${SOURCE_FILES})


find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
INCLUDE_DIRECTORIES(${SDL_IMAGE_INCLUDE_DIRS})

if(NOT SDL_Image_FOUND)
    message(ERROR " SDL_Image not found!")
endif(NOT SDL_Image_FOUND)

find_package(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})

if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)


file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/Data)
file(COPY Data DESTINATION .)
configure_file(bin/glew32.dll glew32.dll COPYONLY)


STRING(REGEX REPLACE "  -mwindows" "" SDL2_LIBRARIES ${SDL2_LIBRARIES})

TARGET_LINK_LIBRARIES(UberGame ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})


