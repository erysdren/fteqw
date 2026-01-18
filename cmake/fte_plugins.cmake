if(NOT FTE_PLUGINS)
	return()
endif()

function(fte_add_plugin name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES")
	add_library(${name} SHARED ${ARG_SOURCES})
	target_compile_options(${name} PRIVATE ${FTE_COMMON_OPTIONS})
	target_compile_definitions(${name} PRIVATE FTEPLUGIN ${FTE_COMMON_DEFINITIONS})
	set_target_properties(${name}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game
			RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game
			SUFFIX ${FTE_SHARED_LIBRARY_SUFFIX}
			PREFIX ""
	)
endfunction()

include(plugins/cod)
