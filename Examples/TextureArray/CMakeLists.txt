set(this_target "TextureArray")

set(TEXTURES 
	"BT_texture.png"
	"BT_jackal.jpg"
	"BT_shrek1.jpg"
	"BT_shrek2.jpg"
	"BT_mike_wazowski.jpg"
	"BT_pepe.jpg"
)

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"texture_array.frag"
	"texture_array.vert"
	#textures
	${TEXTURES}
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)


TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"texture_array.frag"
	"texture_array.vert"
OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Examples/${this_target}"
)

install(FILES ${TEXTURES} 
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