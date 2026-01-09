set(this_target "LayeredTexture")
set(APP_DIR ${BASE_APP_DIR}/${this_target})

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"layered_texture.frag"
	"layered_texture.vert"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)

set_target_properties(${this_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${APP_DIR}/Obj
	ARCHIVE_OUTPUT_DIRECTORY ${APP_DIR}/Obj
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"layered_texture.frag"
	"layered_texture.vert"
SHADERS_DIRECTORY ${this_target}
)
file(COPY Grunge_201M.jpg DESTINATION ${APP_DIR})
file(COPY Soil_120M.jpg DESTINATION ${APP_DIR})
file(COPY Soil_124M.jpg DESTINATION ${APP_DIR})
file(COPY Wood_145M.jpg DESTINATION ${APP_DIR})
file(COPY Wood_285M.jpg DESTINATION ${APP_DIR})
 
#copy RHI.dll into example's dir
add_custom_command(TARGET ${this_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BASE_APP_DIR}/${PROJECT_NAME}.dll
		$<$<CONFIG:Debug>:${BASE_APP_DIR}/${PROJECT_NAME}.pdb>
        ${APP_DIR}/.
)