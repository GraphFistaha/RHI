include(CMakeUtils.cmake)

set(this_target ${PROJECT_NAME})

if (RHI_BUILD_SHARED)
	add_library(${this_target} SHARED)
else()
	add_library(${this_target})
endif()

# ----------------------- build library -------------------------

set(interface_target "RHI_Interface")

add_library(${interface_target} OBJECT)
target_sources (${interface_target}
PUBLIC
	"Public/RHI.hpp"
	"Public/RHI_def.h"
	"Public/Images.hpp"
	"Public/Descriptors.hpp"
	"Public/Utils.hpp"
PRIVATE
	"Private/Images.cpp"
	"Private/ImageTraits.hpp"
	"Private/Types.hpp"
	"Private/OwnedBy.hpp"
	"Private/ObjectsTable.hpp"
	"Private/FastDynamicCast.hpp"
	"Private/Overload.hpp"
	"Private/Hash.hpp"
	"Private/Table2D.hpp"
	"Private/SmallVector.hpp"
	"Private/Constants.hpp"
)

target_compile_definitions(${interface_target} PRIVATE RHI_BUILD)
set_target_properties(${interface_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (${RHI_VULKAN_BACKEND})
	add_subdirectory(Vulkan) # sets backend_target
	target_link_libraries(${backend_target} PRIVATE ${interface_target})
	target_include_directories(${backend_target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
	target_link_libraries(${this_target} PRIVATE ${backend_target})
	target_compile_definitions(${backend_target} PRIVATE RHI_BUILD)
endif()
target_link_libraries(${this_target} PUBLIC ${interface_target})

#add_subdirectory(DirectXImpl)
if (${RHI_BUILD_TESTS})
	add_subdirectory(Tests)
endif()

target_include_directories(${interface_target}
PUBLIC
	"Public"
PRIVATE
	"./"
)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()


target_compile_definitions(${interface_target}
PUBLIC
	$<$<BOOL:${RHI_BUILD_SHARED}>:RHI_BUILD_SHARED>
	$<$<BOOL:${RHI_USE_LOG_OUTPUT}>:RHI_USE_LOG_OUTPUT>
	$<$<CONFIG:Release>:RHI_FAST_DYNAMIC_CAST>
)

install(TARGETS ${this_target}
  EXPORT RHI
  RUNTIME DESTINATION Bin
  LIBRARY DESTINATION Lib
  ARCHIVE DESTINATION Lib
  INCLUDES DESTINATION include
)

install(DIRECTORY Public/ DESTINATION include/)
