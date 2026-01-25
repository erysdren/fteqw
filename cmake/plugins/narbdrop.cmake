if(NOT FTE_PLUGIN_NARBDROP)
	return()
endif()

fte_add_plugin(fteplug_narbdrop
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/fs_ore.c
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/mod_cmf.c
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/narbdrop.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_narbdrop PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/narbdrop ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR} ${FTE_ENGINE_COMMON_DIR})
target_compile_definitions(fteplug_narbdrop PRIVATE MULTITHREAD)
