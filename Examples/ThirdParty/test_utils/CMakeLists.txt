set(this_target "test_utils")

add_library(${this_target})

target_sources (${this_target}
PUBLIC
	"Camera.hpp"
PRIVATE
	"Camera.cpp"
)

target_include_directories(${this_target} PUBLIC "./")

target_link_libraries(${this_target} 
PRIVATE
	glm
)