if(NOT FTE_PLUGIN_HL2)
	return()
endif()

fte_add_plugin(fteplug_hl2
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_vpk.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_vpk_vtmb.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_gma.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/img_tth.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/img_vtf.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mod_hl2.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mat_vmt.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mod_vbsp.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/hl2.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_hl2 PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/hl2 ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR} ${FTE_ENGINE_COMMON_DIR})
target_compile_definitions(fteplug_hl2 PRIVATE MULTITHREAD)
