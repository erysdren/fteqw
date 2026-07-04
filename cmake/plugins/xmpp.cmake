if(NOT FTE_PLUGIN_XMPP)
	return()
endif()

fte_add_plugin(fteplug_xmpp
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/jabber/jabberclient.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/jingle.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/sift.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/xml.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
)
target_include_directories(fteplug_xmpp PRIVATE ${FTE_PLUGINS_ROOT_DIR} ${FTE_PLUGINS_ROOT_DIR}/jabber ${FTE_ENGINE_CLIENT_DIR})
