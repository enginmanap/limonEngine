cmake_minimum_required(VERSION 3.3)
project(customTriggers)

set(PROJECT_VERSION 0.6)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_SILENT_WARNINGS)

set(CMAKE_CXX_STANDARD 14)

IF (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -mconsole -Wall -Wextra -Wnon-virtual-dtor -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wnon-virtual-dtor -pedantic")
ENDIF()

INCLUDE(FindPkgConfig)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(glm REQUIRED)
if (NOT glm_FOUND)
    message(ERROR " GLM not found!")
endif (NOT glm_FOUND)
INCLUDE_DIRECTORIES(SYSTEM ${GLM_INCLUDE_DIRS})

file(GLOB_RECURSE EXTENSION_FILES
        src/*.cpp)

add_library(customTriggers SHARED
#fill with customisation sources
        ${EXTENSION_FILES}
        )

IF (WIN32 AND NOT ( MINGW OR MSYS))
    TARGET_LINK_LIBRARIES(customTriggers ${CMAKE_SOURCE_DIR}/lib/libLimonAPI.lib)
else()
    TARGET_LINK_LIBRARIES(customTriggers ${CMAKE_SOURCE_DIR}/lib/libLimonAPI.a)
ENDIF()

set_target_properties(customTriggers PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(customTriggers PROPERTIES SOVERSION 1)