project(Texturing)

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
	"textures.frag"
	"textures.vert"
SHADERS_DIRECTORY ${PROJECT_NAME}
)
file(COPY texture.png DESTINATION ${APP_DIR})
file(COPY jackal.jpg DESTINATION ${APP_DIR})
file(COPY shrek1.jpg DESTINATION ${APP_DIR})
file(COPY shrek2.jpg DESTINATION ${APP_DIR})
file(COPY mike_wazowski.jpg DESTINATION ${APP_DIR})
file(COPY pepe.jpg DESTINATION ${APP_DIR})