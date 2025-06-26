#include "../plugin.h"

qboolean Plug_PSX_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "thps";

	// set plugin name
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// initialize components
	if (!Plug_PSX_Init()) return false;

	return true;
}
