cmake_minimum_required(VERSION 3.10)
project(LimonEngine)

set(PROJECT_VERSION 0.6)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_SILENT_WARNINGS)
add_definitions(-DASSIMP_VALIDATE_WORKAROUND)
add_definitions(-DCEREAL_SUPPORT)
add_definitions(-DTICK_PER_SECOND=60)
add_definitions(-DMAX_PYTHON_SCRIPT_COUNT=64)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
#include(cotire)

IF (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic -fdiagnostics-all-candidates")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld ${CMAKE_EXE_LINKER_FLAGS}")
    set(WINDOWS_SPECIFIC_LINK_LIBRARIES "imm32")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wnon-virtual-dtor -pedantic")
ENDIF()

IF (UNIX AND ${CMAKE_VERSION} VERSION_GREATER "3.11.0")
    cmake_policy (SET CMP0072 NEW)
ENDIF()

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(LIBS_EXTERNAL_ASSIMP "Provide external assimp library." OFF)
option(LIBS_EXTERNAL_BULLET "Provide external bullet library." OFF)

# suppress new entries if they were deleted and subproject is disabled
if (NOT LIBS_EXTERNAL_ASSIMP)
    option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
    option(ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." OFF)
    option(ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON)
endif()

if (NOT LIBS_EXTERNAL_BULLET)
    OPTION(USE_GRAPHICAL_BENCHMARK "Use Graphical Benchmark" OFF)
    OPTION(ENABLE_VHACD "Use VHACD in BulletRobotics and pybullet" OFF)
    OPTION(BUILD_EGL "Build OpenGL/EGL" OFF)
endif()

include(libs/CmakeLists.txt)
include_directories(libs/ImGui)
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

iF(LIBS_EXTERNAL_BULLET)
    message("Using System Bullet")
    find_package(Bullet REQUIRED)
    INCLUDE_DIRECTORIES(SYSTEM ${BULLET_INCLUDE_DIRS})
ELSE ()
    message("Using local Bullet")
    find_package(Bullet REQUIRED PATH_SUFFIXES ./libs/ )
	include_directories("libs/bullet/src/")
ENDIF ()
if (NOT BULLET_FOUND)
    message(ERROR " Bullet not found!")
endif (NOT BULLET_FOUND)

find_package(glm REQUIRED)
if (NOT glm_FOUND)
    message(ERROR " GLM not found!")
endif (NOT glm_FOUND)
INCLUDE_DIRECTORIES(SYSTEM ${GLM_INCLUDE_DIRS})

if (LIBS_EXTERNAL_ASSIMP)
    set(LIBS_ASSIMP_INCLUDE_DIR "" CACHE PATH "The main include path")
    set(LIBS_ASSIMP_CONFIG_DIR "" CACHE PATH "The include path with generated config.h")
    set(LIBS_ASSIMP_LIBRARY_DIR "" CACHE PATH "")

    # Let's leave the choice of usage between system-provided libraries and custom-built
    if (LIBS_ASSIMP_INCLUDE_DIR STREQUAL "" OR LIBS_ASSIMP_CONFIG_DIR STREQUAL "" OR LIBS_ASSIMP_LIBRARY_DIR STREQUAL "")
        message(STATUS "Using system Assimp")
        find_package(assimp REQUIRED)
        if (NOT assimp_FOUND)
            message(ERROR " Assimp not found!")
        endif ()
        INCLUDE_DIRECTORIES(SYSTEM ${ASSIMP_INCLUDE_DIRS})

        # TODO test on Apple
        if (APPLE)
            set(ASSIMP_LIBRARIES "/usr/local/lib/libassimp.5.dylib")
        endif(APPLE)
    else()
        message(STATUS "Using external Assimp")
        target_include_directories(LimonEngine PRIVATE ${LIBS_ASSIMP_INCLUDE_DIR} ${LIBS_ASSIMP_CONFIG_DIR})
        target_link_directories(LimonEngine PRIVATE ${LIBS_ASSIMP_LIBRARY_DIR})
    endif()
endif()


add_custom_target(copyData
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_BINARY_DIR}/Engine)
add_dependencies(LimonEngine copyData)
target_include_directories(LimonEngine PRIVATE "libs/assimp/include")
TARGET_LINK_LIBRARIES(LimonEngine ImGui ImGuizmo OpenAL meshoptimizer nodeGraph assimp pybind11::embed ${WINDOWS_SPECIFIC_LINK_LIBRARIES} ${TinyXML2_LIBRARIES} ${BULLET_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
target_precompile_headers(LimonEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h)

IF (WIN32)
    #add icon to windows binary
    #SET( LimonEngine_LINKFLAGS "\"${CMAKE_CURRENT_SOURCE_DIR}/resources/limonEngine.res\"" )
    #SET_TARGET_PROPERTIES(LimonEngine PROPERTIES LINK_FLAGS ${LimonEngine_LINKFLAGS} )
ENDIF()

add_library(LimonAPI STATIC
        src/limonAPI/TriggerInterface.cpp
        src/limonAPI/PlayerExtensionInterface.cpp
        src/limonAPI/ActorInterface.cpp
        src/limonAPI/LimonAPI.cpp
        src/limonAPI/InputStates.cpp
        src/limonAPI/LimonConverter.h
        src/limonAPI/LimonTypes.h
        src/Options.cpp
        )

file(GLOB API_HEADERS
        "src/limonAPI/*.h"
        )

file(GLOB API_UTIL_HEADERS
        "src/limonAPI/util/*.h"
)


target_precompile_headers(LimonAPI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h)

set_property(TARGET LimonAPI PROPERTY POSITION_INDEPENDENT_CODE ON)
IF (WIN32 AND NOT ( MINGW OR MSYS))
    add_custom_command(TARGET LimonAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/LimonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E copy ${API_HEADERS} ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/util/
        COMMAND ${CMAKE_COMMAND} -E copy ${API_UTIL_HEADERS} ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/util
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/limonAPI/CMakeLists.txt ${CMAKE_BINARY_DIR}/LimonAPI/
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
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/util/
        COMMAND ${CMAKE_COMMAND} -E copy ${API_UTIL_HEADERS} ${CMAKE_BINARY_DIR}/LimonAPI/include/limonAPI/util
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/limonAPI/CMakeLists.txt ${CMAKE_BINARY_DIR}/LimonAPI/
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

add_library(GraphicsBackend SHARED
        src/limonAPI/Graphics/GraphicsProgram.cpp
        src/limonAPI/Graphics/GraphicsProgramPreprocessor.cpp
        GraphicBackends/OpenGLGraphics.cpp
        src/Assets/GraphicsProgramAsset.cpp
        )
target_precompile_headers(GraphicsBackend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h)

IF (WIN32 AND NOT ( MINGW OR MSYS))
    TARGET_LINK_LIBRARIES(GraphicsBackend ${CMAKE_BINARY_DIR}/libLimonAPI.lib GLEW::GLEW ${OPENGL_LIBRARIES})
else()
    TARGET_LINK_LIBRARIES(GraphicsBackend ${CMAKE_BINARY_DIR}/libLimonAPI.a GLEW::GLEW ${OPENGL_LIBRARIES})
ENDIF()

set_target_properties(GraphicsBackend PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(GraphicsBackend PROPERTIES SOVERSION 1)

add_dependencies(GraphicsBackend LimonAPI)
add_dependencies(LimonEngine GraphicsBackend)
add_dependencies(LimonEngine customTriggers)



#cotire(LimonEngine)
