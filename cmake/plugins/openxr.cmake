if(NOT FTE_PLUGIN_OPENXR)
	return()
endif()

fte_add_plugin(fteplug_openxr
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/openxr.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_openxr PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_ENGINE_CLIENT_DIR})
