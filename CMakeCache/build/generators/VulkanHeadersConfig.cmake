########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(VulkanHeaders_FIND_QUIETLY)
    set(VulkanHeaders_MESSAGE_MODE VERBOSE)
else()
    set(VulkanHeaders_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/VulkanHeadersTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${vulkan-headers_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(VulkanHeaders_VERSION_STRING "1.3.296.0")
set(VulkanHeaders_INCLUDE_DIRS ${vulkan-headers_INCLUDE_DIRS_RELEASE} )
set(VulkanHeaders_INCLUDE_DIR ${vulkan-headers_INCLUDE_DIRS_RELEASE} )
set(VulkanHeaders_LIBRARIES ${vulkan-headers_LIBRARIES_RELEASE} )
set(VulkanHeaders_DEFINITIONS ${vulkan-headers_DEFINITIONS_RELEASE} )


# Definition of extra CMake variables from cmake_extra_variables


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${vulkan-headers_BUILD_MODULES_PATHS_RELEASE} )
    message(${VulkanHeaders_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


