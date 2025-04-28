/*
 * written by erysdren (it/its) in 2025
 */
#include "../plugin.h"

static plugimagefuncs_t *imagefuncs = NULL;

static struct pendingtextureinfo *Image_ReadSTYFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	return NULL;
}

static plugimageloaderfuncs_t styfuncs =
{
	"GBH Style File",
	sizeof(struct pendingtextureinfo),
	true,
	Image_ReadSTYFile,
};

qboolean Plug_STY_Init(void)
{
	imagefuncs = plugfuncs->GetEngineInterface(plugimagefuncs_name, sizeof(*imagefuncs));
	if (!imagefuncs)
		return false;
	return plugfuncs->ExportInterface(plugimageloaderfuncs_name, &styfuncs, sizeof(styfuncs));
}
