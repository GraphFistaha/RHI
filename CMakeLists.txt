cmake_minimum_required(VERSION 3.25)

option(RHI_BUILD_EXAMPLES "Build example programs" ON)
option(RHI_BUILD_TESTS "Build unit tests" ON)
option(RHI_BUILD_SHARED "Build RHI as shared library" ON)
option(RHI_VULKAN_BACKEND "use vulkan as backend" ON)

project(RHI C CXX)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDATD 17)
cmake_policy(SET CMP0135 NEW)
if(LINUX)
add_compile_options(-fPIC)
endif()
if (${RHI_BUILD_TESTS})
    enable_testing()
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "Build/${CMAKE_BUILD_TYPE}" CACHE PATH "Installation directory" FORCE)
endif()

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/Examples)

include(${SOURCE_DIR}/ShaderCompile.cmake)
# -------------------- Install conan packages ----------------------
message(STATUS "Copy conanfile.txt into CMake cache")
configure_file("${PROJECT_SOURCE_DIR}/conanfile.txt" "${CMAKE_BINARY_DIR}/conanfile.txt" COPYONLY)

include(${SOURCE_DIR}/conan.cmake)

conan_cmake_autodetect(settings RUNTIME static)
message(status ${settings})
conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings}
                    OUTPUT_FOLDER ${CMAKE_BINARY_DIR}/ConanResult
)

file(GLOB_RECURSE ToolchainFile
     "${CMAKE_BINARY_DIR}/ConanResult/**/conan_toolchain.cmake"
)
get_filename_component(GeneratorsFolder ${ToolchainFile} DIRECTORY)

list(APPEND CMAKE_MODULE_PATH ${GeneratorsFolder})
list(APPEND CMAKE_PREFIX_PATH ${GeneratorsFolder})

set(dir ${GeneratorsFolder})
FILE(GLOB genFiles RELATIVE ${dir} ${dir}/*)
message(STATUS "Generated files in ${dir} = ${genFiles}")

#------------------------------------------------------------------


ADD_SUBDIRECTORY(${SOURCE_DIR})
if (${RHI_BUILD_EXAMPLES})
	add_subdirectory(${EXAMPLES_DIR})
endif(${RHI_BUILD_EXAMPLES})