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

find_package(glm CONFIG REQUIRED)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	glm::glm
	test_utils
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

#copy RHI.dll into example's dir
add_custom_command(TARGET ${this_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BASE_APP_DIR}/${PROJECT_NAME}.dll
		$<$<CONFIG:Debug>:${BASE_APP_DIR}/${PROJECT_NAME}.pdb>
        ${APP_DIR}/.
)