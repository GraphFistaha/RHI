include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

message(STATUS "Fetch glslcc")
if (WIN32)
	FetchContent_Declare(
	  glslcc
	  URL      https://github.com/septag/glslcc/releases/download/v1.7.6/glslcc-1.7.6-win64.zip
	  URL_HASH MD5=efe6909448f7e3a0d41c919845ec4204
	)
	set(GLSLCC_EXECUTABLE "glslcc.exe")
elseif(UNIX)
	FetchContent_Declare(
	  glslcc
	  URL      https://github.com/septag/glslcc/releases/download/v1.7.6/glslcc-1.7.6-linux.tar.gz
	  URL_HASH MD5=25e9ce6c48ca8809309fd2cba4d186fd
	)
	set(GLSLCC_EXECUTABLE "glslcc")
elseif(APPLE)
	FetchContent_Declare(
	  glslcc
	  URL      https://github.com/septag/glslcc/releases/download/v1.7.6/glslcc-1.7.6-osx.tar.gz
	  URL_HASH MD5=ddf357aa9d631d7ba4f1da9628a8e08f
	)
	set(GLSLCC_EXECUTABLE "glslcc")
endif()
FetchContent_MakeAvailable(glslcc)


FUNCTION(TARGET_PRECOMPILE_SHADERS)
	set(oneValueArgs TARGET API OUTPUT_DIRECTORY)
	set(multiValueArgs SHADERS)
	cmake_parse_arguments(PARSED_ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	string(TOLOWER ${PARSED_ARG_API} API_NAME)

	get_target_property(TARGET_DIR ${PARSED_ARG_TARGET} SOURCE_DIR)
	set(RESULT_FOLDER ${PARSED_ARG_OUTPUT_DIRECTORY})

	message(STATUS "Shaders found for ${PARSED_ARG_TARGET}; TargetDir = ${TARGET_DIR};
					backend = ${PARSED_ARG_API}; OutputDir = ${RESULT_FOLDER}")
	file(MAKE_DIRECTORY ${RESULT_FOLDER})

	if (${API_NAME} STREQUAL "vulkan")
		foreach(ShaderFile ${PARSED_ARG_SHADERS})
			set(RESULT_FILENAME ${ShaderFile})
			STRING(REPLACE ".vert" "_vert.spv" RESULT_FILENAME ${RESULT_FILENAME})
			STRING(REPLACE ".frag" "_frag.spv" RESULT_FILENAME ${RESULT_FILENAME})
			STRING(REPLACE ".geom" "_geom.spv" RESULT_FILENAME ${RESULT_FILENAME})
			add_custom_command(
				TARGET ${PARSED_ARG_TARGET}
				POST_BUILD
				COMMAND glslc "${ShaderFile}" "-o" "${RESULT_FOLDER}/${RESULT_FILENAME}"
				COMMENT "Compiling ${ShaderFile}..."
				WORKING_DIRECTORY ${TARGET_DIR}
			)
		endforeach()
	else()
		# TODO: Make it for other backends
	endif()
ENDFUNCTION(TARGET_PRECOMPILE_SHADERS)