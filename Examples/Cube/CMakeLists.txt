set(this_target "Cube")

set(TEXTURES
	"container.png"
	"floor.jpg"
)

add_executable(${this_target} 
	"main.cpp"
	"Renderer.hpp"
	"Renderer.cpp"
	#shaders
	"cube.vert"
	"cube.frag"
	"cube.geom"
	# textures
	${TEXTURES}
)

find_package(glm CONFIG REQUIRED)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	glm::glm
	test_utils
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"cube.frag"
	"cube.vert"
	"cube.geom"
OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Examples/${this_target}"
)

install(FILES 
	${TEXTURES}
	DESTINATION Examples/${this_target}
)

install(TARGETS ${this_target}
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)

install(TARGETS RHI
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)