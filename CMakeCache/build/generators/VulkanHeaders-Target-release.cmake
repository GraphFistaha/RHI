# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(vulkan-headers_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(vulkan-headers_FRAMEWORKS_FOUND_RELEASE "${vulkan-headers_FRAMEWORKS_RELEASE}" "${vulkan-headers_FRAMEWORK_DIRS_RELEASE}")

set(vulkan-headers_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET vulkan-headers_DEPS_TARGET)
    add_library(vulkan-headers_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET vulkan-headers_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${vulkan-headers_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${vulkan-headers_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### vulkan-headers_DEPS_TARGET to all of them
conan_package_library_targets("${vulkan-headers_LIBS_RELEASE}"    # libraries
                              "${vulkan-headers_LIB_DIRS_RELEASE}" # package_libdir
                              "${vulkan-headers_BIN_DIRS_RELEASE}" # package_bindir
                              "${vulkan-headers_LIBRARY_TYPE_RELEASE}"
                              "${vulkan-headers_IS_HOST_WINDOWS_RELEASE}"
                              vulkan-headers_DEPS_TARGET
                              vulkan-headers_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "vulkan-headers"    # package_name
                              "${vulkan-headers_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${vulkan-headers_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT Vulkan::Registry #############

        set(vulkan-headers_Vulkan_Registry_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(vulkan-headers_Vulkan_Registry_FRAMEWORKS_FOUND_RELEASE "${vulkan-headers_Vulkan_Registry_FRAMEWORKS_RELEASE}" "${vulkan-headers_Vulkan_Registry_FRAMEWORK_DIRS_RELEASE}")

        set(vulkan-headers_Vulkan_Registry_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET vulkan-headers_Vulkan_Registry_DEPS_TARGET)
            add_library(vulkan-headers_Vulkan_Registry_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET vulkan-headers_Vulkan_Registry_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_DEPENDENCIES_RELEASE}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'vulkan-headers_Vulkan_Registry_DEPS_TARGET' to all of them
        conan_package_library_targets("${vulkan-headers_Vulkan_Registry_LIBS_RELEASE}"
                              "${vulkan-headers_Vulkan_Registry_LIB_DIRS_RELEASE}"
                              "${vulkan-headers_Vulkan_Registry_BIN_DIRS_RELEASE}" # package_bindir
                              "${vulkan-headers_Vulkan_Registry_LIBRARY_TYPE_RELEASE}"
                              "${vulkan-headers_Vulkan_Registry_IS_HOST_WINDOWS_RELEASE}"
                              vulkan-headers_Vulkan_Registry_DEPS_TARGET
                              vulkan-headers_Vulkan_Registry_LIBRARIES_TARGETS
                              "_RELEASE"
                              "vulkan-headers_Vulkan_Registry"
                              "${vulkan-headers_Vulkan_Registry_NO_SONAME_MODE_RELEASE}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Vulkan::Registry
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_LIBRARIES_TARGETS}>
                     )

        if("${vulkan-headers_Vulkan_Registry_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Vulkan::Registry
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         vulkan-headers_Vulkan_Registry_DEPS_TARGET)
        endif()

        set_property(TARGET Vulkan::Registry APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_LINKER_FLAGS_RELEASE}>)
        set_property(TARGET Vulkan::Registry APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_INCLUDE_DIRS_RELEASE}>)
        set_property(TARGET Vulkan::Registry APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_LIB_DIRS_RELEASE}>)
        set_property(TARGET Vulkan::Registry APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_COMPILE_DEFINITIONS_RELEASE}>)
        set_property(TARGET Vulkan::Registry APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_RELEASE}>)


    ########## COMPONENT Vulkan::Headers #############

        set(vulkan-headers_Vulkan_Headers_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(vulkan-headers_Vulkan_Headers_FRAMEWORKS_FOUND_RELEASE "${vulkan-headers_Vulkan_Headers_FRAMEWORKS_RELEASE}" "${vulkan-headers_Vulkan_Headers_FRAMEWORK_DIRS_RELEASE}")

        set(vulkan-headers_Vulkan_Headers_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET vulkan-headers_Vulkan_Headers_DEPS_TARGET)
            add_library(vulkan-headers_Vulkan_Headers_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET vulkan-headers_Vulkan_Headers_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_DEPENDENCIES_RELEASE}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'vulkan-headers_Vulkan_Headers_DEPS_TARGET' to all of them
        conan_package_library_targets("${vulkan-headers_Vulkan_Headers_LIBS_RELEASE}"
                              "${vulkan-headers_Vulkan_Headers_LIB_DIRS_RELEASE}"
                              "${vulkan-headers_Vulkan_Headers_BIN_DIRS_RELEASE}" # package_bindir
                              "${vulkan-headers_Vulkan_Headers_LIBRARY_TYPE_RELEASE}"
                              "${vulkan-headers_Vulkan_Headers_IS_HOST_WINDOWS_RELEASE}"
                              vulkan-headers_Vulkan_Headers_DEPS_TARGET
                              vulkan-headers_Vulkan_Headers_LIBRARIES_TARGETS
                              "_RELEASE"
                              "vulkan-headers_Vulkan_Headers"
                              "${vulkan-headers_Vulkan_Headers_NO_SONAME_MODE_RELEASE}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Vulkan::Headers
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_LIBRARIES_TARGETS}>
                     )

        if("${vulkan-headers_Vulkan_Headers_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Vulkan::Headers
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         vulkan-headers_Vulkan_Headers_DEPS_TARGET)
        endif()

        set_property(TARGET Vulkan::Headers APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_LINKER_FLAGS_RELEASE}>)
        set_property(TARGET Vulkan::Headers APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_INCLUDE_DIRS_RELEASE}>)
        set_property(TARGET Vulkan::Headers APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_LIB_DIRS_RELEASE}>)
        set_property(TARGET Vulkan::Headers APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_COMPILE_DEFINITIONS_RELEASE}>)
        set_property(TARGET Vulkan::Headers APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_RELEASE}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET vulkan-headers::vulkan-headers APPEND PROPERTY INTERFACE_LINK_LIBRARIES Vulkan::Registry)
    set_property(TARGET vulkan-headers::vulkan-headers APPEND PROPERTY INTERFACE_LINK_LIBRARIES Vulkan::Headers)

########## For the modules (FindXXX)
set(vulkan-headers_LIBRARIES_RELEASE vulkan-headers::vulkan-headers)
