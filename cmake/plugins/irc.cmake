if(NOT FTE_PLUGIN_IRC)
	return()
endif()

fte_add_plugin(fteplug_irc
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/irc/ircclient.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_irc PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/irc ${FTE_ENGINE_CLIENT_DIR})
