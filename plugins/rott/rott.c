#include "../plugin.h"

qboolean FSROTT_Init(void);
qboolean ModROTT_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "rott";
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	if (!FSROTT_Init()) return false;
	if (!ModROTT_Init()) return false;

	return true;
}
