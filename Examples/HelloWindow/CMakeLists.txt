set(this_target HelloWindow)

add_executable(${this_target} 
	"main.cpp"
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)

install(TARGETS ${this_target}
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)

install(TARGETS RHI
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)