# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanMemoryAllocator-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${vulkan-memory-allocator_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${VulkanMemoryAllocator_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET GPUOpen::VulkanMemoryAllocator)
    add_library(GPUOpen::VulkanMemoryAllocator INTERFACE IMPORTED)
    message(${VulkanMemoryAllocator_MESSAGE_MODE} "Conan: Target declared 'GPUOpen::VulkanMemoryAllocator'")
endif()
if(NOT TARGET VulkanMemoryAllocator)
    add_library(VulkanMemoryAllocator INTERFACE IMPORTED)
    set_property(TARGET VulkanMemoryAllocator PROPERTY INTERFACE_LINK_LIBRARIES GPUOpen::VulkanMemoryAllocator)
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanMemoryAllocator-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()