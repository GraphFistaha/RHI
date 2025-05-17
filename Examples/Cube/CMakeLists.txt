set(this_target "Cube")
set(APP_DIR ${BASE_APP_DIR}/${this_target})

add_executable(${this_target} 
	"main.cpp"
	"Renderer.hpp"
	"Renderer.cpp"
	#shaders
	"cube.vert"
	"cube.frag"
	"cube.geom"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	glfw
	glm
	test_utils
)

target_include_directories(${this_target}
PRIVATE
	"../ThirdParty/stb"
)

set_target_properties(${this_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${APP_DIR}/Obj
	ARCHIVE_OUTPUT_DIRECTORY ${APP_DIR}/Obj
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"cube.frag"
	"cube.vert"
	"cube.geom"
)

file(COPY "container.png" DESTINATION ${APP_DIR})
file(COPY "floor.jpg" DESTINATION ${APP_DIR})