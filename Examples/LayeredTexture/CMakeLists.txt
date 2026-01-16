set(this_target "LayeredTexture")

set(TEXTURES
	"Grunge_201M.jpg"
	"Soil_120M.jpg"
	"Soil_124M.jpg"
	"Wood_145M.jpg"
	"Wood_285M.jpg"
)

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"layered_texture.frag"
	"layered_texture.vert"
	# textures
	${TEXTURES}
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"layered_texture.frag"
	"layered_texture.vert"
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