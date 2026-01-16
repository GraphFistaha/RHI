########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(vk-bootstrap_COMPONENT_NAMES "")
if(DEFINED vk-bootstrap_FIND_DEPENDENCY_NAMES)
  list(APPEND vk-bootstrap_FIND_DEPENDENCY_NAMES VulkanHeaders)
  list(REMOVE_DUPLICATES vk-bootstrap_FIND_DEPENDENCY_NAMES)
else()
  set(vk-bootstrap_FIND_DEPENDENCY_NAMES VulkanHeaders)
endif()
set(VulkanHeaders_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(vk-bootstrap_PACKAGE_FOLDER_RELEASE "C:/Users/rozhkov/.conan2/p/vk-bocfbacbb636735/p")
set(vk-bootstrap_BUILD_MODULES_PATHS_RELEASE )


set(vk-bootstrap_INCLUDE_DIRS_RELEASE "${vk-bootstrap_PACKAGE_FOLDER_RELEASE}/include")
set(vk-bootstrap_RES_DIRS_RELEASE )
set(vk-bootstrap_DEFINITIONS_RELEASE )
set(vk-bootstrap_SHARED_LINK_FLAGS_RELEASE )
set(vk-bootstrap_EXE_LINK_FLAGS_RELEASE )
set(vk-bootstrap_OBJECTS_RELEASE )
set(vk-bootstrap_COMPILE_DEFINITIONS_RELEASE )
set(vk-bootstrap_COMPILE_OPTIONS_C_RELEASE )
set(vk-bootstrap_COMPILE_OPTIONS_CXX_RELEASE )
set(vk-bootstrap_LIB_DIRS_RELEASE "${vk-bootstrap_PACKAGE_FOLDER_RELEASE}/lib")
set(vk-bootstrap_BIN_DIRS_RELEASE )
set(vk-bootstrap_LIBRARY_TYPE_RELEASE STATIC)
set(vk-bootstrap_IS_HOST_WINDOWS_RELEASE 1)
set(vk-bootstrap_LIBS_RELEASE vk-bootstrap)
set(vk-bootstrap_SYSTEM_LIBS_RELEASE )
set(vk-bootstrap_FRAMEWORK_DIRS_RELEASE )
set(vk-bootstrap_FRAMEWORKS_RELEASE )
set(vk-bootstrap_BUILD_DIRS_RELEASE )
set(vk-bootstrap_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(vk-bootstrap_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${vk-bootstrap_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${vk-bootstrap_COMPILE_OPTIONS_C_RELEASE}>")
set(vk-bootstrap_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${vk-bootstrap_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${vk-bootstrap_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${vk-bootstrap_EXE_LINK_FLAGS_RELEASE}>")


set(vk-bootstrap_COMPONENTS_RELEASE )