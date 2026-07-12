if(NOT FTE_PLUGIN_SATURN)
	return()
endif()

fte_add_plugin(fteplug_saturn
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/saturn/saturn.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_saturn PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/saturn ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
