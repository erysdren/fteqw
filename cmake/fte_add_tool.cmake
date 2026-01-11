
function(fte_add_tool name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES")
	add_executable(${name} ${ARG_SOURCES})
	set_target_properties(${name}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
			RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
	)
endfunction()
