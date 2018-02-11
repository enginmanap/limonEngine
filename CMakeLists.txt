cmake_minimum_required(VERSION 3.3)
project(LimonEngine)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(cotire)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic")
IF (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic")
ENDIF()

set(SOURCE_FILES src/main.cpp src/SDL2Helper.cpp src/SDL2Helper.h src/GLHelper.cpp src/GLHelper.h src/Model.cpp src/Model.h src/World.cpp src/World.h src/InputHandler.cpp src/InputHandler.h src/Camera.cpp src/Camera.h src/SkyBox.cpp src/SkyBox.h src/Assets/TextureAsset.cpp src/Assets/TextureAsset.h src/Assets/CubeMapAsset.cpp src/Assets/CubeMapAsset.h src/GLSLProgram.cpp src/GLSLProgram.h src/Renderable.cpp src/Renderable.h src/Utils/GLMConverter.cpp src/Utils/GLMConverter.h src/BulletDebugDrawer.cpp src/BulletDebugDrawer.h src/GUI/GUIText.cpp src/GUI/GUIText.h src/GUI/GUILayer.cpp src/GUI/GUILayer.h src/PhysicalRenderable.cpp src/PhysicalRenderable.h src/GUI/GUIRenderable.cpp src/GUI/GUIRenderable.h src/FontManager.cpp src/FontManager.h src/GUI/GUIFPSCounter.cpp src/GUI/GUIFPSCounter.h src/Utils/AssimpUtils.cpp src/Utils/AssimpUtils.h src/Light.cpp src/Light.h src/Material.cpp src/Material.h src/Assets/AssetManager.cpp src/Assets/AssetManager.h src/Assets/Asset.cpp src/Assets/Asset.h src/Assets/ModelAsset.cpp src/Assets/ModelAsset.h src/Assets/MeshAsset.cpp src/Assets/MeshAsset.h src/Assets/BoneNode.cpp src/Assets/BoneNode.h src/Utils/GLMUtils.h src/Options.h src/GUI/GUITextDynamic.cpp src/GUI/GUITextDynamic.h src/Utils/Logger.cpp src/Utils/Logger.h src/AI/Actor.cpp src/AI/Actor.h src/AI/HumanEnemy.h src/AI/AIMovementGrid.cpp)

add_executable(LimonEngine ${SOURCE_FILES})

INCLUDE(FindPkgConfig)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED SDL2_image>=2.0.0)

include_directories(${SDL2_INCLUDE_DIR}
                    ${SDL2_IMAGE_INCLUDE_DIR})

if (NOT SDL2_IMAGE_FOUND)
    message(ERROR " SDL2_Image not found!")
endif (NOT SDL2_IMAGE_FOUND)

if(WINDOWS)
    find_package(FREETYPE REQUIRED)
else()
    find_package(Freetype REQUIRED)
endif()
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

if (NOT FREETYPE_FOUND)
    message(ERROR " FREETYPE not found!")
endif (NOT FREETYPE_FOUND)

find_package(TinyXML2 REQUIRED)
if (NOT TinyXML2_FOUND)
    message(ERROR " TinyXML2 not found!")
endif (NOT TinyXML2_FOUND)
INCLUDE_DIRECTORIES(${TinyXML2_INCLUDE_DIRS})

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
add_dependencies(LimonEngine copyData)

TARGET_LINK_LIBRARIES(LimonEngine ${TinyXML2_LIBRARIES} ${BULLET_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${ASSIMP_LIBRARIES})
cotire(LimonEngine)