if(NOT FTE_PLUGIN_BULLET)
	return()
endif()

fte_add_plugin(fteplug_bullet
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/bullet/bulletplug.cpp
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_bullet PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/bullet ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_link_libraries(fteplug_bullet PRIVATE BulletDynamics BulletCollision LinearMath)
target_include_directories(fteplug_bullet PRIVATE ${bullet_SOURCE_DIR}/src)
