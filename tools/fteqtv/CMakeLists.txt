project(fteqtv)
if(NOT FTE_TOOL_QTV)
	return()
endif()
add_executable(fteqtv
	${PROJECT_SOURCE_DIR}/libqtvc/glibc_sucks.c
	${PROJECT_SOURCE_DIR}/libqtvc/msvc_sucks.c
	${PROJECT_SOURCE_DIR}/netchan.c
	${PROJECT_SOURCE_DIR}/parse.c
	${PROJECT_SOURCE_DIR}/msg.c
	${PROJECT_SOURCE_DIR}/qw.c
	${PROJECT_SOURCE_DIR}/source.c
	${PROJECT_SOURCE_DIR}/bsp.c
	${PROJECT_SOURCE_DIR}/rcon.c
	${PROJECT_SOURCE_DIR}/mdfour.c
	${PROJECT_SOURCE_DIR}/crc.c
	${PROJECT_SOURCE_DIR}/control.c
	${PROJECT_SOURCE_DIR}/forward.c
	${PROJECT_SOURCE_DIR}/pmove.c
	${PROJECT_SOURCE_DIR}/menu.c
	${PROJECT_SOURCE_DIR}/httpsv.c
	${PROJECT_SOURCE_DIR}/relay.c
	${FTE_ENGINE_COMMON_DIR}/sha1.c
	${FTE_ENGINE_COMMON_DIR}/md5.c
)
target_link_libraries(fteqtv PRIVATE ZLIB::ZLIB $<TARGET_NAME_IF_EXISTS:Math::Math>)
target_include_directories(fteqtv PRIVATE ${FTE_ENGINE_CLIENT_DIR})
set_target_properties(fteqtv
	PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
		RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
)
