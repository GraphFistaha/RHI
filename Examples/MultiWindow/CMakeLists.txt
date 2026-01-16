set(this_target "MultiWindow")

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"triangle_quad.frag"
	"triangle.vert"
	"quad.vert"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"triangle_quad.frag"
	"triangle.vert"
	"quad.vert"
OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Examples/${this_target}"
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