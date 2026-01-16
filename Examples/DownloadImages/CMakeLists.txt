set(this_target "DownloadImages")

add_executable(${this_target} 
 "main.cpp" 
 "mike_wazowski.jpg"
)

find_package(stb CONFIG REQUIRED)

target_link_libraries(${this_target} 
PRIVATE
	RHI
    stb::stb
    test_utils
)

install(FILES 
    "mike_wazowski.jpg"
    DESTINATION Examples/${this_target}
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