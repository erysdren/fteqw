if(NOT FTE_PLUGIN_JOLT)
	return()
endif()

FetchContent_Declare(JoltPhysics GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics" GIT_TAG "v5.5.0" SOURCE_SUBDIR "Build")
FetchContent_MakeAvailable(JoltPhysics)

fte_add_plugin(fteplug_jolt
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/jolt/jolt.cpp
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_jolt PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/jolt ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_link_libraries(fteplug_jolt PRIVATE Jolt)
