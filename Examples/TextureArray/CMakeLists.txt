project(TextureArray)

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
	"texture_array.frag"
	"texture_array.vert"
SHADERS_DIRECTORY ${PROJECT_NAME}
)
file(COPY BT_texture.png DESTINATION ${APP_DIR})
file(COPY BT_jackal.jpg DESTINATION ${APP_DIR})
file(COPY BT_shrek1.jpg DESTINATION ${APP_DIR})
file(COPY BT_shrek2.jpg DESTINATION ${APP_DIR})
file(COPY BT_mike_wazowski.jpg DESTINATION ${APP_DIR})
file(COPY BT_pepe.jpg DESTINATION ${APP_DIR})