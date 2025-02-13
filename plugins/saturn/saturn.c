/*
 * written by erysdren (it/its) in 2025
 *
 * based on work by Rich Whitehouse and vfig
 * https://www.richwhitehouse.com/index.php?postid=68
 */
#include "../plugin.h"
#include "../engine/common/com_mesh.h"
#include "../engine/common/com_bih.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugmodfuncs_t *modfuncs = NULL;

typedef struct lev_header {
	quint32_t mUnknown1; //always 0?
	quint32_t mUnknown2; //might represent a bank or collective size
	quint32_t mNodeCount; //28 bytes each
	quint32_t mPlaneCount; //40 bytes each
	quint32_t mVertCount; //8 bytes each
	quint32_t mQuadCount; //5 bytes each
	quint32_t mTileTextureDataSize;
	quint32_t mTileEntryCount; //44 bytes each
	quint32_t mTileColorDataSize;
	quint32_t mEntityCount; //4 bytes each
	quint32_t mEntityDataSize;
	quint32_t mEntityPolyLinkCount; //unverified - 18 bytes each
	quint32_t mEntityPolyLinkData1Count; //2 bytes each
	quint32_t mEntityPolyLinkData2Count; //4 bytes each
	quint32_t mUnknownCount; //16 bytes each
} lev_header_t;

static qboolean QDECL Mod_LoadSaturnLEV(struct model_s *mod, void *buffer, size_t fsize)
{
	return false;
}

qboolean Plug_Init(void)
{
	qboolean status = true;
	char plugname[] = "saturn";

	/* get funcs */
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(plugfsfuncs_t));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(plugmodfuncs_t));
	if (!filefuncs || !modfuncs)
		return false;

	/* register plugin */
	modfuncs->RegisterModelFormatText("Sega Saturn Quake Level (.lev)", ".lev", Mod_LoadSaturnLEV);

	/* set plugin name */
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	return status;
}
