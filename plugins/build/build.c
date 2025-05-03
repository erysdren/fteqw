#include "../plugin.h"

qboolean GRP_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "build";

	// set plugin name
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// initialize components
	if (!GRP_Init()) return false;

	return true;
}
