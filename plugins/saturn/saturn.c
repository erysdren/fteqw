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

/* plugins are silly huh */
quint32_t SwapU32(quint32_t l)
{
	return
		((l>>24)&0x000000ff)|
		((l>> 8)&0x0000ff00)|
		((l<< 8)&0x00ff0000)|
		((l<<24)&0xff000000);
}

#if defined(FTE_BIG_ENDIAN)
#define BigU32(x) x
#else
#define BigU32(x) SwapU32(x)
#endif

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
	int i;
	lev_header_t *header;

	/* skip sky data to header */
	header = (lev_header_t *)((qbyte *)buffer + 131104);

	/* swap header data */
	for (i = 0; i < 15; i++)
		*((quint32_t *)header + i) = BigU32(*((quint32_t *)header + i));

	mod->type = mod_alias;

	return true;
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
