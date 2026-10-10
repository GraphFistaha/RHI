cmake_minimum_required(VERSION 3.25)

option(RHI_BUILD_EXAMPLES "Build example programs" ON)
option(RHI_BUILD_TESTS "Build unit tests" ON)
option(RHI_BUILD_SHARED "Build RHI as shared library" ON)
option(RHI_VULKAN_BACKEND "use vulkan as backend" ON)
option(RHI_USE_LOG_OUTPUT "use debug log output" ON)

project(RHI C CXX)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDARD 17)
cmake_policy(SET CMP0135 NEW)
if(LINUX)
add_compile_options(-fPIC)
endif()
if (${RHI_BUILD_TESTS})
    enable_testing()
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}" CACHE PATH "Installation directory" FORCE)
endif()
message(STATUS "Install directory is ${CMAKE_INSTALL_PREFIX}")

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/Examples)

include(${SOURCE_DIR}/ShaderCompile.cmake)

ADD_SUBDIRECTORY(${SOURCE_DIR})
if (${RHI_BUILD_EXAMPLES})
	add_subdirectory(${EXAMPLES_DIR})
endif(${RHI_BUILD_EXAMPLES})
