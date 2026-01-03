include(../CMakeUtils.cmake)

set(this_target ${PROJECT_NAME})

if (RHI_BUILD_SHARED)
	add_library(${this_target} SHARED)
else()
	add_library(${this_target})
endif()

# ----------------------- build library -------------------------

target_sources (${this_target}
PUBLIC
	"RHI.hpp"
	"RHI_def.h"
	"Headers/Images.hpp"
PRIVATE
	"MultibackendUtils/ImageTraits.hpp"
	"MultibackendUtils/Types.hpp"
	"MultibackendUtils/OwnedBy.hpp"
	"MultibackendUtils/ObjectsTable.hpp"
)

if (${RHI_VULKAN_BACKEND})
add_subdirectory(Vulkan)
endif()
#add_subdirectory(DirectXImpl)

target_include_directories(${this_target} 
PUBLIC
   "./"
   "MultibackendUtils"
)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()


target_compile_definitions(${this_target}
PRIVATE
	RHI_BUILD
	$<$<BOOL:${RHI_BUILD_SHARED}>:RHI_BUILD_SHARED>
)

if (${RHI_BUILD_EXAMPLES})
	SUBDIRLIST(SUBDIRS ${BASE_APP_DIR})
	foreach(EXAMPLE_DIR ${SUBDIRS})
		if (NOT ${EXAMPLE_DIR} STREQUAL "Obj")
			message(STATUS "Add command to copy RHI.dll to ${EXAMPLE_DIR}")
			add_custom_command(TARGET ${this_target} POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy
							${BASE_APP_DIR}/${PROJECT_NAME}.dll
							$<$<CONFIG:Debug>:${BASE_APP_DIR}/${PROJECT_NAME}.pdb>
							${BASE_APP_DIR}/${EXAMPLE_DIR}/.
					)
		endif()
	endforeach()
				
endif()
