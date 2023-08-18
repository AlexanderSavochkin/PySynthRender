cmake_minimum_required (VERSION 3.20)
cmake_policy(VERSION 3.0)

project(SyntheticGeran)

set(CMAKE_CXX_STANDARD 17) # this does nothing for MSVC, use target_compile_options below
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
find_package(glm REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(glfw3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(assimp REQUIRED)
message(STATUS "Found assimp in ${ASSIMP_INCLUDE_DIR}")
find_package(PythonLibs 3.10 REQUIRED)
message(STATUS "Found PythonLibs in ${PYTHON_INCLUDE_DIRS}")
find_package(Boost COMPONENTS python REQUIRED)
message(STATUS "Found Boost.Python in ${PYTHON_INCLUDE_DIRS}")
find_package(Boost COMPONENTS numpy REQUIRED)
message(STATUS "Found Boost.Numpy in ${PYTHON_INCLUDE_DIRS}")


set(SOURCES
    src/SynthRenderer.cpp
    src/glad.c
    src/stb_image.cpp
    src/QuickHull.cpp
    src/PythonWrapper.cpp
)


add_library(SynthRenderer SHARED ${SOURCES})
set_target_properties(SynthRenderer PROPERTIES PREFIX "")
target_link_libraries(SynthRenderer glfw ${CMAKE_DL_LIBS} assimp ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
target_include_directories(SynthRenderer PRIVATE ${PYTHON_INCLUDE_DIRS}) 

