set(this_target "TextureArray")
set(APP_DIR ${BASE_APP_DIR}/${this_target})

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"texture_array.frag"
	"texture_array.vert"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	glfw
)

set_target_properties(${this_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${APP_DIR}/Obj
	ARCHIVE_OUTPUT_DIRECTORY ${APP_DIR}/Obj
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"texture_array.frag"
	"texture_array.vert"
SHADERS_DIRECTORY ${this_target}
)
file(COPY BT_texture.png DESTINATION ${APP_DIR})
file(COPY BT_jackal.jpg DESTINATION ${APP_DIR})
file(COPY BT_shrek1.jpg DESTINATION ${APP_DIR})
file(COPY BT_shrek2.jpg DESTINATION ${APP_DIR})
file(COPY BT_mike_wazowski.jpg DESTINATION ${APP_DIR})
file(COPY BT_pepe.jpg DESTINATION ${APP_DIR})