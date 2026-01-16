########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(glm_FIND_QUIETLY)
    set(glm_MESSAGE_MODE VERBOSE)
else()
    set(glm_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/glmTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${glm_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(glm_VERSION_STRING "1.0.1")
set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIRS_RELEASE} )
set(glm_INCLUDE_DIR ${glm_INCLUDE_DIRS_RELEASE} )
set(glm_LIBRARIES ${glm_LIBRARIES_RELEASE} )
set(glm_DEFINITIONS ${glm_DEFINITIONS_RELEASE} )


# Definition of extra CMake variables from cmake_extra_variables


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${glm_BUILD_MODULES_PATHS_RELEASE} )
    message(${glm_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


