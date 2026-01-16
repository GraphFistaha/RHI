# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/vk-bootstrap-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${vk-bootstrap_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${vk-bootstrap_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET vk-bootstrap::vk-bootstrap)
    add_library(vk-bootstrap::vk-bootstrap INTERFACE IMPORTED)
    message(${vk-bootstrap_MESSAGE_MODE} "Conan: Target declared 'vk-bootstrap::vk-bootstrap'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/vk-bootstrap-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()