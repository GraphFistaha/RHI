########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(vulkan-memory-allocator_COMPONENT_NAMES "")
if(DEFINED vulkan-memory-allocator_FIND_DEPENDENCY_NAMES)
  list(APPEND vulkan-memory-allocator_FIND_DEPENDENCY_NAMES VulkanHeaders)
  list(REMOVE_DUPLICATES vulkan-memory-allocator_FIND_DEPENDENCY_NAMES)
else()
  set(vulkan-memory-allocator_FIND_DEPENDENCY_NAMES VulkanHeaders)
endif()
set(VulkanHeaders_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(vulkan-memory-allocator_PACKAGE_FOLDER_RELEASE "C:/Users/rozhkov/.conan2/p/vulka5f8c9f361cfa0/p")
set(vulkan-memory-allocator_BUILD_MODULES_PATHS_RELEASE )


set(vulkan-memory-allocator_INCLUDE_DIRS_RELEASE "${vulkan-memory-allocator_PACKAGE_FOLDER_RELEASE}/include")
set(vulkan-memory-allocator_RES_DIRS_RELEASE )
set(vulkan-memory-allocator_DEFINITIONS_RELEASE )
set(vulkan-memory-allocator_SHARED_LINK_FLAGS_RELEASE )
set(vulkan-memory-allocator_EXE_LINK_FLAGS_RELEASE )
set(vulkan-memory-allocator_OBJECTS_RELEASE )
set(vulkan-memory-allocator_COMPILE_DEFINITIONS_RELEASE )
set(vulkan-memory-allocator_COMPILE_OPTIONS_C_RELEASE )
set(vulkan-memory-allocator_COMPILE_OPTIONS_CXX_RELEASE )
set(vulkan-memory-allocator_LIB_DIRS_RELEASE )
set(vulkan-memory-allocator_BIN_DIRS_RELEASE )
set(vulkan-memory-allocator_LIBRARY_TYPE_RELEASE UNKNOWN)
set(vulkan-memory-allocator_IS_HOST_WINDOWS_RELEASE 1)
set(vulkan-memory-allocator_LIBS_RELEASE )
set(vulkan-memory-allocator_SYSTEM_LIBS_RELEASE )
set(vulkan-memory-allocator_FRAMEWORK_DIRS_RELEASE )
set(vulkan-memory-allocator_FRAMEWORKS_RELEASE )
set(vulkan-memory-allocator_BUILD_DIRS_RELEASE )
set(vulkan-memory-allocator_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(vulkan-memory-allocator_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${vulkan-memory-allocator_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${vulkan-memory-allocator_COMPILE_OPTIONS_C_RELEASE}>")
set(vulkan-memory-allocator_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${vulkan-memory-allocator_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${vulkan-memory-allocator_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${vulkan-memory-allocator_EXE_LINK_FLAGS_RELEASE}>")


set(vulkan-memory-allocator_COMPONENTS_RELEASE )