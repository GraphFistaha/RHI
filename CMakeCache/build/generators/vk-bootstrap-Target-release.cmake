# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(vk-bootstrap_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(vk-bootstrap_FRAMEWORKS_FOUND_RELEASE "${vk-bootstrap_FRAMEWORKS_RELEASE}" "${vk-bootstrap_FRAMEWORK_DIRS_RELEASE}")

set(vk-bootstrap_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET vk-bootstrap_DEPS_TARGET)
    add_library(vk-bootstrap_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET vk-bootstrap_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${vk-bootstrap_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${vk-bootstrap_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:vulkan-headers::vulkan-headers>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### vk-bootstrap_DEPS_TARGET to all of them
conan_package_library_targets("${vk-bootstrap_LIBS_RELEASE}"    # libraries
                              "${vk-bootstrap_LIB_DIRS_RELEASE}" # package_libdir
                              "${vk-bootstrap_BIN_DIRS_RELEASE}" # package_bindir
                              "${vk-bootstrap_LIBRARY_TYPE_RELEASE}"
                              "${vk-bootstrap_IS_HOST_WINDOWS_RELEASE}"
                              vk-bootstrap_DEPS_TARGET
                              vk-bootstrap_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "vk-bootstrap"    # package_name
                              "${vk-bootstrap_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${vk-bootstrap_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${vk-bootstrap_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${vk-bootstrap_LIBRARIES_TARGETS}>
                 )

    if("${vk-bootstrap_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET vk-bootstrap::vk-bootstrap
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     vk-bootstrap_DEPS_TARGET)
    endif()

    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${vk-bootstrap_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${vk-bootstrap_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${vk-bootstrap_LIB_DIRS_RELEASE}>)
    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${vk-bootstrap_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET vk-bootstrap::vk-bootstrap
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${vk-bootstrap_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(vk-bootstrap_LIBRARIES_RELEASE vk-bootstrap::vk-bootstrap)
