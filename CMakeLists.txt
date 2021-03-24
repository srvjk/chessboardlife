cmake_minimum_required (VERSION 3.12)

set (PROJECT chessboardlife)

project (${PROJECT})

cmake_policy(SET CMP0074 NEW)

file (GLOB_RECURSE HEADERS "src/*.h")
file (GLOB_RECURSE SOURCES "src/*.cpp")

project (${PROJECT})

set (PLATFORM_WINDOWS FALSE)
set (PLATFORM_LINUX FALSE)

if (TARGET_PLATFORM STREQUAL "win")
    set (PLATFORM_WINDOWS TRUE)
    add_definitions (-DPLATFORM_WINDOWS)
elseif (TARGET_PLATFORM STREQUAL "linux")
    set (PLATFORM_LINUX TRUE)
    add_definitions (-DPLATFORM_LINUX)
    add_compile_options (-Wno-trigraphs -Wall
                         -Wno-switch
                         -Wno-unused-function
                         -Wno-unused-variable
                         -Wno-sign-compare
                         -Wno-reorder
                         -Wno-parentheses
                         -Wno-unused-but-set-variable
    )
else ()
    message (FATAL_ERROR "Unknown or unsupported target platform!")
endif ()

add_definitions (-DCHESSBOARDLIFE_LIB)

find_package (Boost 1.74.0 REQUIRED)

# OpenGL
find_package (OpenGL REQUIRED)
if (NOT OPENGL_FOUND)
    message (ERROR "OpenGL not found!")
else ()
    message (STATUS "OpenGL found.")
endif (NOT OPENGL_FOUND)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

# set output path for executables ("exe" and ".dll" files)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# set output directories for libraries (".so" files)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

include_directories (../basis/INCLUDE)
include_directories (${Boost_INCLUDE_DIRS})
include_directories (sfml/include)

link_directories (${Boost_LIBRARY_DIRS})
link_directories (../basis/LIB/${TARGET})
link_directories (sfml/lib)

if (CONFIGURATION STREQUAL debug)
    set (SFML_SFX -d)
endif ()

set (SFML_LIBS
    sfml-graphics${SFML_SFX}
    sfml-window${SFML_SFX}
    sfml-system${SFML_SFX}
    )

add_library (${PROJECT} MODULE ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT} basis ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} ${SFML_LIBS})
