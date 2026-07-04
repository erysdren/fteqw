if(NOT FTE_PLUGIN_XSV)
	return()
endif()

fte_add_plugin(fteplug_xsv
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/xsv/m_x.c
		${FTE_PLUGINS_ROOT_DIR}/xsv/x_reqs.c
		${FTE_PLUGINS_ROOT_DIR}/xsv/x_res.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_xsv PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/xsv ${FTE_ENGINE_CLIENT_DIR})
