set(this_target "test_utils")

add_library(${this_target})

target_sources (${this_target}
PUBLIC
	"Camera.hpp"
	"Window.hpp"
	"TestUtils.hpp"
	"test_utils_def.h"
PRIVATE
	"Camera.cpp"
 "Window.cpp"
 "TestUtils.cpp"
)

target_include_directories(${this_target} PUBLIC "./")

target_link_libraries(${this_target} 
PRIVATE
	glm
	glfw
	RHI
)

target_include_directories(${this_target}
PRIVATE
	"../stb"
)

target_compile_definitions(${this_target}
PRIVATE
	TEST_UTILS_BUILD
)