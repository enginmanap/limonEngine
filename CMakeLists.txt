cmake_minimum_required(VERSION 3.3)
project(LimonEngine)

set(PROJECT_VERSION 0.6)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_SILENT_WARNINGS)
add_definitions(-DASSIMP_VALIDATE_WORKAROUND)
add_definitions(-DCEREAL_SUPPORT)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(cotire)

IF (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wnon-virtual-dtor -pedantic")
ENDIF()

include(libs/CmakeLists.txt)

include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCE_FILES
        src/*.cpp)

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
INCLUDE_DIRECTORIES(SYSTEM ${BULLET_INCLUDE_DIRS})

if (NOT BULLET_FOUND)
    message(ERROR " Bullet not found!")
endif (NOT BULLET_FOUND)

find_package(glm REQUIRED)
if (NOT glm_FOUND)
    message(ERROR " GLM not found!")
endif (NOT glm_FOUND)
INCLUDE_DIRECTORIES(SYSTEM ${GLM_INCLUDE_DIRS})

find_package(assimp REQUIRED)
if (NOT assimp_FOUND)
    message(ERROR " Assimp not found!")
endif (NOT assimp_FOUND)
INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})

add_custom_target(copyData
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_BINARY_DIR}/Engine)
add_dependencies(LimonEngine copyData)

TARGET_LINK_LIBRARIES(LimonEngine ImGui ImGuizmo OpenAL nodeGraph ${TinyXML2_LIBRARIES} ${BULLET_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${ASSIMP_LIBRARIES})

IF (WIN32)
    #add icon to windows binary
    SET( LimonEngine_LINKFLAGS ${CMAKE_CURRENT_SOURCE_DIR}/resources/limonEngine.res )
    SET_TARGET_PROPERTIES(LimonEngine PROPERTIES LINK_FLAGS ${LimonEngine_LINKFLAGS} )
ENDIF()

add_library(LimonAPI STATIC
        src/API/TriggerInterface.cpp
        src/API/PlayerExtensionInterface.cpp
        src/API/ActorInterface.cpp
        src/API/LimonAPI.cpp
        src/API/InputStates.cpp
        src/API/LimonConverter.h
        )

file(GLOB API_HEADERS
        "src/API/*.h"
        )
set_property(TARGET LimonAPI PROPERTY POSITION_INDEPENDENT_CODE ON)
IF (WIN32 AND NOT ( MINGW OR MSYS))
    add_custom_command(TARGET LimonAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/LimonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E copy ${API_HEADERS} ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/API/CMakeLists.txt ${CMAKE_BINARY_DIR}/LimonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libLimonAPI.lib ${CMAKE_BINARY_DIR}/LimonAPI/lib
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/src
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "LimonAPI.zip" --format=zip -- ${CMAKE_BINARY_DIR}/LimonAPI
        COMMAND echo "Building api project done"
    )
else()
    add_custom_command(TARGET LimonAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/LimonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E copy ${API_HEADERS} ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/API/CMakeLists.txt ${CMAKE_BINARY_DIR}/LimonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libLimonAPI.a ${CMAKE_BINARY_DIR}/LimonAPI/lib
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/src
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "LimonAPI.zip" --format=zip -- ${CMAKE_BINARY_DIR}/LimonAPI
        COMMAND echo "Building api project done"
    )
ENDIF()

add_library(customTriggers SHARED
        samples/CoinPickUpOnTrigger.cpp
        samples/MayanCoinPickup.cpp
        samples/MayanLever.cpp
        samples/DoorAnimationAction.cpp
        samples/ShooterPlayerExtension.cpp
        samples/HumanEnemy.cpp
        samples/CowboyShooterExtension.cpp
        samples/CowboyEnemyAI.cpp
        samples/WesternMenuPlayerExtension.cpp
        samples/WesternMenuPlayerExtension.h
        samples/KillCowboyPlayer.cpp
        samples/WesternStoryStartupAction.cpp
        samples/WesternStoryAtGraveAction.cpp
        samples/WesternStoryAtUndertakerAction.cpp
        samples/WesternStoryAtSaloonAction.cpp
        samples/WesternStoryAtTrainAction.cpp
        samples/WesternStoryNewGameAction.cpp
        samples/SSAOKernelRenderMethod.cpp
        )

IF (WIN32 AND NOT ( MINGW OR MSYS))
    TARGET_LINK_LIBRARIES(customTriggers ${CMAKE_BINARY_DIR}/libLimonAPI.lib)
else()
    TARGET_LINK_LIBRARIES(customTriggers ${CMAKE_BINARY_DIR}/libLimonAPI.a)
ENDIF()

add_dependencies(customTriggers LimonAPI)


set_target_properties(customTriggers PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(customTriggers PROPERTIES SOVERSION 1)

#cotire(LimonEngine)
