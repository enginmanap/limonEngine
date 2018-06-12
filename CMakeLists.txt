cmake_minimum_required(VERSION 3.3)
project(LimonEngine)

set(PROJECT_VERSION 0.4)


#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(cotire)

IF (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wnon-virtual-dtor -pedantic")
ENDIF()

include(libs/CmakeLists.txt)

set(SOURCE_FILES src/Utils/Logger.cpp src/Utils/Logger.h src/ImGuiHelper.cpp src/ImGuiHelper.h src/main.cpp src/SDL2Helper.cpp src/SDL2Helper.h src/GLHelper.cpp src/GLHelper.h src/GameObjects/Model.cpp src/GameObjects/Model.h src/World.cpp src/World.h src/InputHandler.cpp src/InputHandler.h src/Camera.cpp src/Camera.h src/GameObjects/SkyBox.cpp src/GameObjects/SkyBox.h src/Assets/TextureAsset.cpp src/Assets/TextureAsset.h src/Assets/CubeMapAsset.cpp src/Assets/CubeMapAsset.h src/GLSLProgram.cpp src/GLSLProgram.h src/Renderable.h src/Utils/GLMConverter.cpp src/Utils/GLMConverter.h src/BulletDebugDrawer.cpp src/BulletDebugDrawer.h src/GUI/GUIText.cpp src/GUI/GUIText.h src/GUI/GUILayer.cpp src/GUI/GUILayer.h src/PhysicalRenderable.cpp src/PhysicalRenderable.h src/GUI/GUIRenderable.cpp src/GUI/GUIRenderable.h src/FontManager.cpp src/FontManager.h src/GUI/GUIFPSCounter.cpp src/GUI/GUIFPSCounter.h src/Utils/AssimpUtils.cpp src/Utils/AssimpUtils.h src/GameObjects/Light.cpp src/GameObjects/Light.h src/Material.cpp src/Material.h src/Assets/AssetManager.cpp src/Assets/AssetManager.h src/Assets/Asset.cpp src/Assets/Asset.h src/Assets/ModelAsset.cpp src/Assets/ModelAsset.h src/Assets/MeshAsset.cpp src/Assets/MeshAsset.h src/Assets/BoneNode.cpp src/Assets/BoneNode.h src/Utils/GLMUtils.h src/Options.h src/GUI/GUITextDynamic.cpp src/GUI/GUITextDynamic.h src/AI/Actor.cpp src/AI/Actor.h src/AI/HumanEnemy.h src/AI/AIMovementGrid.cpp src/GameObjects/Players/PhysicalPlayer.cpp src/GameObjects/Players/PhysicalPlayer.h src/CameraAttachment.h src/GameObjects/Players/FreeMovingPlayer.cpp src/GameObjects/Players/FreeMovingPlayer.h src/GameObjects/Players/FreeCursorPlayer.cpp src/GameObjects/Players/FreeCursorPlayer.cpp src/GameObjects/Players/Player.h src/GameObjects/GameObject.h src/WorldLoader.cpp src/WorldLoader.h src/WorldSaver.cpp src/WorldSaver.h src/GameObjects/TriggerObject.cpp src/GameObjects/TriggerObject.h src/Transformation.cpp src/Assets/Animations/AnimationAssimp.h src/Assets/Animations/AnimationAssimp.cpp src/Assets/Animations/AnimationLoader.h src/Assets/Animations/AnimationLoader.cpp src/Assets/Animations/AnimationNode.cpp src/Assets/Animations/AnimationNode.h src/Assets/Animations/AnimationCustom.cpp src/Assets/Animations/AnimationCustom.h src/GamePlay/LimonAPI.h src/GamePlay/LimonAPI.cpp src/GamePlay/TriggerInterface.h src/GamePlay/AnimateOnTrigger.cpp src/GamePlay/AnimateOnTrigger.h src/GamePlay/AddGuiTextOnTrigger.cpp src/GamePlay/AddGuiTextOnTrigger.h src/GamePlay/TriggerInterface.cpp src/GamePlay/RemoveGuiTextOnTrigger.h src/GamePlay/RemoveGuiTextOnTrigger.cpp src/AnimationSequencer.cpp src/AnimationSequencer.h)

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

add_custom_target(copyData
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Data ${CMAKE_BINARY_DIR}/Data)
add_dependencies(LimonEngine copyData)

TARGET_LINK_LIBRARIES(LimonEngine ImGui ImGuizmo ${TinyXML2_LIBRARIES} ${BULLET_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${ASSIMP_LIBRARIES})

IF (WIN32)
    #add icon to windows binary
    SET( LimonEngine_LINKFLAGS ${CMAKE_CURRENT_SOURCE_DIR}/resources/limonEngine.res )
    SET_TARGET_PROPERTIES(LimonEngine PROPERTIES LINK_FLAGS ${LimonEngine_LINKFLAGS} )
ENDIF()

add_library(customTriggers SHARED src/GamePlay/TriggerInterface.cpp src/GamePlay/LimonAPI.cpp src/GamePlay/CoinPickUpOnTrigger.cpp src/GamePlay/UpdateGuiTextOnTrigger)

TARGET_LINK_LIBRARIES(customTriggers ${TinyXML2_LIBRARIES})

set_target_properties(customTriggers PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(customTriggers PROPERTIES SOVERSION 1)

cotire(LimonEngine)