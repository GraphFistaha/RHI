########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND vulkan-headers_COMPONENT_NAMES Vulkan::Headers Vulkan::Registry)
list(REMOVE_DUPLICATES vulkan-headers_COMPONENT_NAMES)
if(DEFINED vulkan-headers_FIND_DEPENDENCY_NAMES)
  list(APPEND vulkan-headers_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES vulkan-headers_FIND_DEPENDENCY_NAMES)
else()
  set(vulkan-headers_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(vulkan-headers_PACKAGE_FOLDER_RELEASE "C:/Users/rozhkov/.conan2/p/vulkaaf4d40321f065/p")
set(vulkan-headers_BUILD_MODULES_PATHS_RELEASE )


set(vulkan-headers_INCLUDE_DIRS_RELEASE "${vulkan-headers_PACKAGE_FOLDER_RELEASE}/res/vulkan/registry"
			"${vulkan-headers_PACKAGE_FOLDER_RELEASE}/include")
set(vulkan-headers_RES_DIRS_RELEASE "${vulkan-headers_PACKAGE_FOLDER_RELEASE}/res")
set(vulkan-headers_DEFINITIONS_RELEASE )
set(vulkan-headers_SHARED_LINK_FLAGS_RELEASE )
set(vulkan-headers_EXE_LINK_FLAGS_RELEASE )
set(vulkan-headers_OBJECTS_RELEASE )
set(vulkan-headers_COMPILE_DEFINITIONS_RELEASE )
set(vulkan-headers_COMPILE_OPTIONS_C_RELEASE )
set(vulkan-headers_COMPILE_OPTIONS_CXX_RELEASE )
set(vulkan-headers_LIB_DIRS_RELEASE )
set(vulkan-headers_BIN_DIRS_RELEASE )
set(vulkan-headers_LIBRARY_TYPE_RELEASE UNKNOWN)
set(vulkan-headers_IS_HOST_WINDOWS_RELEASE 1)
set(vulkan-headers_LIBS_RELEASE )
set(vulkan-headers_SYSTEM_LIBS_RELEASE )
set(vulkan-headers_FRAMEWORK_DIRS_RELEASE )
set(vulkan-headers_FRAMEWORKS_RELEASE )
set(vulkan-headers_BUILD_DIRS_RELEASE )
set(vulkan-headers_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(vulkan-headers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${vulkan-headers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${vulkan-headers_COMPILE_OPTIONS_C_RELEASE}>")
set(vulkan-headers_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${vulkan-headers_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${vulkan-headers_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${vulkan-headers_EXE_LINK_FLAGS_RELEASE}>")


set(vulkan-headers_COMPONENTS_RELEASE Vulkan::Headers Vulkan::Registry)
########### COMPONENT Vulkan::Registry VARIABLES ############################################

set(vulkan-headers_Vulkan_Registry_INCLUDE_DIRS_RELEASE "${vulkan-headers_PACKAGE_FOLDER_RELEASE}/res/vulkan/registry")
set(vulkan-headers_Vulkan_Registry_LIB_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Registry_BIN_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Registry_LIBRARY_TYPE_RELEASE UNKNOWN)
set(vulkan-headers_Vulkan_Registry_IS_HOST_WINDOWS_RELEASE 1)
set(vulkan-headers_Vulkan_Registry_RES_DIRS_RELEASE "${vulkan-headers_PACKAGE_FOLDER_RELEASE}/res")
set(vulkan-headers_Vulkan_Registry_DEFINITIONS_RELEASE )
set(vulkan-headers_Vulkan_Registry_OBJECTS_RELEASE )
set(vulkan-headers_Vulkan_Registry_COMPILE_DEFINITIONS_RELEASE )
set(vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_C_RELEASE "")
set(vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_CXX_RELEASE "")
set(vulkan-headers_Vulkan_Registry_LIBS_RELEASE )
set(vulkan-headers_Vulkan_Registry_SYSTEM_LIBS_RELEASE )
set(vulkan-headers_Vulkan_Registry_FRAMEWORK_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Registry_FRAMEWORKS_RELEASE )
set(vulkan-headers_Vulkan_Registry_DEPENDENCIES_RELEASE )
set(vulkan-headers_Vulkan_Registry_SHARED_LINK_FLAGS_RELEASE )
set(vulkan-headers_Vulkan_Registry_EXE_LINK_FLAGS_RELEASE )
set(vulkan-headers_Vulkan_Registry_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(vulkan-headers_Vulkan_Registry_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${vulkan-headers_Vulkan_Registry_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${vulkan-headers_Vulkan_Registry_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${vulkan-headers_Vulkan_Registry_EXE_LINK_FLAGS_RELEASE}>
)
set(vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${vulkan-headers_Vulkan_Registry_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Vulkan::Headers VARIABLES ############################################

set(vulkan-headers_Vulkan_Headers_INCLUDE_DIRS_RELEASE "${vulkan-headers_PACKAGE_FOLDER_RELEASE}/include")
set(vulkan-headers_Vulkan_Headers_LIB_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Headers_BIN_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Headers_LIBRARY_TYPE_RELEASE UNKNOWN)
set(vulkan-headers_Vulkan_Headers_IS_HOST_WINDOWS_RELEASE 1)
set(vulkan-headers_Vulkan_Headers_RES_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Headers_DEFINITIONS_RELEASE )
set(vulkan-headers_Vulkan_Headers_OBJECTS_RELEASE )
set(vulkan-headers_Vulkan_Headers_COMPILE_DEFINITIONS_RELEASE )
set(vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_C_RELEASE "")
set(vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_CXX_RELEASE "")
set(vulkan-headers_Vulkan_Headers_LIBS_RELEASE )
set(vulkan-headers_Vulkan_Headers_SYSTEM_LIBS_RELEASE )
set(vulkan-headers_Vulkan_Headers_FRAMEWORK_DIRS_RELEASE )
set(vulkan-headers_Vulkan_Headers_FRAMEWORKS_RELEASE )
set(vulkan-headers_Vulkan_Headers_DEPENDENCIES_RELEASE )
set(vulkan-headers_Vulkan_Headers_SHARED_LINK_FLAGS_RELEASE )
set(vulkan-headers_Vulkan_Headers_EXE_LINK_FLAGS_RELEASE )
set(vulkan-headers_Vulkan_Headers_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(vulkan-headers_Vulkan_Headers_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${vulkan-headers_Vulkan_Headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${vulkan-headers_Vulkan_Headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${vulkan-headers_Vulkan_Headers_EXE_LINK_FLAGS_RELEASE}>
)
set(vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${vulkan-headers_Vulkan_Headers_COMPILE_OPTIONS_C_RELEASE}>")