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
)

if (${RHI_VULKAN_BACKEND})
add_subdirectory(Vulkan)
endif()
#add_subdirectory(DirectXImpl)

target_include_directories(${this_target} 
PUBLIC
	"Public"
PRVATE
	"./"
)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()


target_compile_definitions(${this_target}
PRIVATE
	RHI_BUILD
	$<$<BOOL:${RHI_BUILD_SHARED}>:RHI_BUILD_SHARED>
)

install(TARGETS ${this_target}
  EXPORT RHI
  RUNTIME DESTINATION Bin
  LIBRARY DESTINATION Lib
  ARCHIVE DESTINATION Lib
  INCLUDES DESTINATION include
)

install(DIRECTORY Public/ DESTINATION include/)
