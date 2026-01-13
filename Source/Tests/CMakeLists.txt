set(this_target ${PROJECT_NAME}_Tests)

add_executable(${this_target})

# ----------------------- build library -------------------------

target_sources (${this_target}
PUBLIC
	"common.cpp"
)

find_package(Catch2 REQUIRED)

target_link_libraries(${this_target}
PRIVATE
	Catch2::Catch2WithMain
	RHI
)

add_test(NAME ${this_target} COMMAND $<TARGET_FILE:${this_target}>)
