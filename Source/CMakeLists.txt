add_library(${PROJECT_NAME} SHARED)

# ----------------------- build library -------------------------

target_sources (${PROJECT_NAME}
PUBLIC
	"RHI.hpp"
	"RHI_def.h"
	"Headers/Images.hpp"
PRIVATE
	"MultibackendUtils/ImageTraits.hpp"
	"MultibackendUtils/Types.hpp"
	"MultibackendUtils/OwnedBy.hpp"
)

if (${RHI_VULKAN_BACKEND})
add_subdirectory(Vulkan)
endif()
#add_subdirectory(DirectXImpl)

target_include_directories(${PROJECT_NAME} 
PUBLIC
   "./"
   "MultibackendUtils"
)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()


target_compile_definitions(${PROJECT_NAME}
PRIVATE
	RHI_BUILD
)
