if(NOT FTE_PLUGIN_ODE)
	return()
endif()

fte_add_plugin(fteplug_ode
	SOURCES
		${FTE_ENGINE_COMMON_DIR}/com_phys_ode.c
		${FTE_ENGINE_COMMON_DIR}/mathlib.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_ode PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_link_libraries(fteplug_ode PRIVATE ODE)
target_compile_definitions(fteplug_ode PRIVATE ODE_STATIC)
