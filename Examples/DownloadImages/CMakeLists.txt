set(this_target "DownloadImages")
set(APP_DIR ${BASE_APP_DIR}/${this_target})

add_executable(${this_target} 
 "main.cpp" 
 "stb_image.h"
 "stb_image_write.h"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
)

set_target_properties(${this_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${APP_DIR}/Obj
	ARCHIVE_OUTPUT_DIRECTORY ${APP_DIR}/Obj
)

file(COPY mike_wazowski.jpg DESTINATION ${APP_DIR})