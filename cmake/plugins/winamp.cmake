if(NOT FTE_PLUGIN_WINAMP)
	return()
endif()

fte_add_plugin(fteplug_winamp
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/winamp/winamp.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_winamp PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/winamp ${FTE_ENGINE_CLIENT_DIR})
