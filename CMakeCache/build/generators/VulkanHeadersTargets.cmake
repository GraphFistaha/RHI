# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanHeaders-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${vulkan-headers_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${VulkanHeaders_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET vulkan-headers::vulkan-headers)
    add_library(vulkan-headers::vulkan-headers INTERFACE IMPORTED)
    message(${VulkanHeaders_MESSAGE_MODE} "Conan: Target declared 'vulkan-headers::vulkan-headers'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanHeaders-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()