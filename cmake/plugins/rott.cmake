if(NOT FTE_PLUGIN_ROTT)
	return()
endif()

fte_add_plugin(fteplug_rott
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/rott/fs_rott.c
		${FTE_PLUGINS_ROOT_DIR}/rott/mod_rott.c
		${FTE_PLUGINS_ROOT_DIR}/rott/rott.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_rott PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/rott ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR} ${FTE_ENGINE_COMMON_DIR})
target_compile_definitions(fteplug_rott PRIVATE MULTITHREAD)
