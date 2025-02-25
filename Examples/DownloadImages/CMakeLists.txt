project(DownloadImages)

add_executable(${PROJECT_NAME} 
 "main.cpp" 
 "stb_image.h"
 "stb_image_write.h"
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
	RHI
)

file(COPY mike_wazowski.jpg DESTINATION ${APP_DIR})