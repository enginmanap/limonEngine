cmake_minimum_required(VERSION 3.3)
project(UberGame)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp src/SDL2Helper.cpp src/SDL2Helper.h src/GLHelper.cpp src/GLHelper.h src/Model.cpp src/Model.h src/World.cpp src/World.h src/InputHandler.cpp src/InputHandler.h src/Camera.cpp src/Camera.h src/SkyBox.cpp src/SkyBox.h src/Assets/TextureAsset.cpp src/Assets/TextureAsset.h src/Assets/CubeMapAsset.cpp src/Assets/CubeMapAsset.h src/GLSLProgram.cpp src/GLSLProgram.h src/Renderable.cpp src/Renderable.h src/Utils/GLMConverter.cpp src/Utils/GLMConverter.h src/BulletDebugDrawer.cpp src/BulletDebugDrawer.h src/GUIText.cpp src/GUIText.h src/GUILayer.cpp src/GUILayer.h src/PhysicalRenderable.cpp src/PhysicalRenderable.h src/GUIRenderable.cpp src/GUIRenderable.h src/FontManager.cpp src/FontManager.h src/GUIFPSCounter.cpp src/GUIFPSCounter.h src/Utils/AssimpUtils.cpp src/Utils/AssimpUtils.h src/Light.cpp src/Light.h src/Material.cpp src/Material.h src/Assets/AssetManager.cpp src/Assets/AssetManager.h src/Assets/Asset.cpp src/Assets/Asset.h src/Assets/ModelAsset.cpp src/Assets/ModelAsset.h src/Assets/MeshAsset.cpp src/Assets/MeshAsset.h src/Assets/BoneNode.cpp src/Assets/BoneNode.h)
add_executable(UberGame ${SOURCE_FILES})

find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})

INCLUDE(FindPkgConfig)

find_package(SDL2_image REQUIRED SDL2_image>=2.0.0)
INCLUDE_DIRECTORIES(${SDL_IMAGE_INCLUDE_DIRS})

if (NOT SDL_Image_FOUND)
    message(ERROR " SDL_Image not found!")
endif (NOT SDL_Image_FOUND)

find_package(FREETYPE REQUIRED)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

if (NOT FREETYPE_FOUND)
    message(ERROR " FREETYPE not found!")
endif (NOT FREETYPE_FOUND)

find_package(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})

if (NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif (NOT OPENGL_FOUND)

find_package(Bullet REQUIRED)
INCLUDE_DIRECTORIES(${BULLET_INCLUDE_DIRS})

if (NOT BULLET_FOUND)
    message(ERROR " Bullet not found!")
endif (NOT BULLET_FOUND)

find_package(assimp REQUIRED)
if (NOT assimp_FOUND)
    message(ERROR " Assimp not found!")
endif (NOT assimp_FOUND)
INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})


#file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/Data)
#MESSAGE(STATUS "CMAKE_BINARY_DIR:         " ${CMAKE_BINARY_DIR})
#file(COPY Data DESTINATION ${CMAKE_BINARY_DIR})

add_custom_target(copyData
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Data ${CMAKE_BINARY_DIR}/Data)
add_dependencies(UberGame copyData)

TARGET_LINK_LIBRARIES(UberGame ${BULLET_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${SDL2IMAGE_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${ASSIMP_LIBRARIES})


