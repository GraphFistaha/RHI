project(DepthStencilTest)

add_executable(${PROJECT_NAME} 
	"main.cpp"
	"stb_image.h"
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
	RHI
	glfw
)

TARGET_PRECOMPILE_SHADERS(TARGET ${PROJECT_NAME} API Vulkan
SHADERS
	"textured_quad.frag"
	"textured_quad.vert"
SHADERS_DIRECTORY ${PROJECT_NAME}
)
file(COPY texture1.png DESTINATION ${APP_DIR})
file(COPY texture2.png DESTINATION ${APP_DIR})