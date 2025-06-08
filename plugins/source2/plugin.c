#include "../plugin.h"

qboolean VMAP_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "source2";

	// set plugin name
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// initialize components
	if (!VMAP_Init()) return false;

	return true;
}
