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

#copy RHI.dll into example's dir
add_custom_command(TARGET ${this_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BASE_APP_DIR}/${PROJECT_NAME}.dll
        $<$<CONFIG:Debug>:${BASE_APP_DIR}/${PROJECT_NAME}.pdb>
        ${APP_DIR}/.
)