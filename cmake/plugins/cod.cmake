if(NOT FTE_PLUGIN_COD)
	return()
endif()

fte_add_plugin(fteplug_cod
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/cod/codmod.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codbsp.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codmat.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codiwi.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_cod PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/cod ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_compile_definitions(fteplug_cod PRIVATE MULTITHREAD)
