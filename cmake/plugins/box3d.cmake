if(NOT FTE_PLUGIN_BOX3D)
	return()
endif()

fte_add_plugin(fteplug_box3d
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/box3d/box3d.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_box3d PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/box3d ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_link_libraries(fteplug_box3d PRIVATE box3d::box3d)
