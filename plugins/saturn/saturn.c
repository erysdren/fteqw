/*
 * written by erysdren (it/its) in 2025
 *
 * based on work by Rich Whitehouse and vfig
 * https://www.richwhitehouse.com/index.php?postid=68
 */
#include "../plugin.h"
#include "../engine/common/com_mesh.h"
#include "../engine/common/com_bih.h"

static plugmodfuncs_t *modfuncs = NULL;

// plugins are silly huh
#ifdef FTEPLUGIN
#undef ZG_Malloc
#define ZG_Malloc(g,s) plugfuncs->GMalloc(g,s)
#undef Z_Malloc
#define Z_Malloc(s) plugfuncs->Malloc(s)
#undef Z_Free
#define Z_Free(p) plugfuncs->Free(p)
#undef Z_Realloc
#define Z_Realloc(p,s) plugfuncs->Realloc(p,s)
#undef Mod_AccumulateTextureVectors
#define Mod_AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms) modfuncs->AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms)
#undef Mod_NormaliseTextureVectors
#define Mod_NormaliseTextureVectors(n,s,t,v,calcnorms) modfuncs->NormaliseTextureVectors(n,s,t,v,calcnorms)
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = FLT_MAX;
	maxs[0] = maxs[1] = maxs[2] = -FLT_MAX;
}
#endif

static void ShortSwapArray(short *l, size_t n)
{
	for (size_t i = 0; i < n; i++)
		l[i] = ShortSwap(l[i]);
}

static void LongSwapArray(int *l, size_t n)
{
	for (size_t i = 0; i < n; i++)
		l[i] = LongSwap(l[i]);
}

#ifdef FTE_BIG_ENDIAN
#define BigShortArray(x,n)
#define BigLongArray(x,n)
#else
#define BigShortArray(x,n) ShortSwapArray(x,n)
#define BigLongArray(x,n) LongSwapArray(x,n)
#endif

#pragma pack(push,1)
// 60 bytes
typedef struct levheader {
	uint32_t mUnknown1; //always 0?
	uint32_t mUnknown2; //might represent a bank or collective size
	uint32_t mNodeCount; //28 bytes each
	uint32_t mPlaneCount; //40 bytes each
	uint32_t mVertCount; //8 bytes each
	uint32_t mQuadCount; //5 bytes each
	uint32_t mTileTextureDataSize;
	uint32_t mTileEntryCount; //44 bytes each
	uint32_t mTileColorDataSize;
	uint32_t mEntityCount; //4 bytes each
	uint32_t mEntityDataSize;
	uint32_t mEntityPolyLinkCount; //unverified - 18 bytes each
	uint32_t mEntityPolyLinkData1Count; //2 bytes each
	uint32_t mEntityPolyLinkData2Count; //4 bytes each
	uint32_t mUnknownCount; //16 bytes each
} levheader_t;

// 28 bytes
typedef struct levnode {
	int16_t mResv[2]; //always 0
	int16_t mPos[3];
	int16_t mDistance; //unverified, could be some kind of angle or axis offset
	uint16_t mFirstPlane;
	uint16_t mLastPlane;
	int16_t mUnknown[6];
} levnode_t;

// 40 bytes
typedef struct levplane {
	uint16_t mVertIndices[4]; //vertices defining the plane
	uint16_t mNodeIndex; //or 0xFFFF, used for triggers/links from planes
	uint16_t mFlags; //typically 256, used largely for poly-objects like movers
	uint16_t mCollisionFlags; //haven't mapped these out
	uint16_t mTileIndex; //0xFFFF if no tile data present
	uint16_t mUnknownIndex; //seems rarely used (usually 0xFFFF)
	uint16_t mQuadStartIndex; //> end index if N/A
	uint16_t mQuadEndIndex;
	uint16_t mVertStartIndex; //first vert referenced by quads
	uint16_t mVertEndIndex;
	int16_t mPlane[4]; //fixed-format normal and distance
	int16_t mAngle; //seems to be an angle about some axis
	uint16_t mResv[2]; //always 0
} levplane_t;

// 44 bytes
typedef struct levtile {
	uint16_t mTextureDataOfs; //byte offset into kLS_TileTextureData
	uint8_t mWidth;
	uint8_t mHeight;
	uint16_t mColorDataOfs; //byte offset into kLS_TileColorData
	int32_t mTileHorzVec[3];
	int32_t mTileVertVec[3];
	int32_t mTileBaseVec[3];
	uint16_t mUnknown;
} levtile_t;

// 8 bytes
typedef struct levvertex {
	int16_t mPosition[3];
	uint8_t mColorValue;
	uint8_t mUnknown;
} levvertex_t;

// 5 bytes
typedef struct levquad {
	uint8_t mIndices[4];
	uint8_t mTextureIndex;
} levquad_t;

// 8 bytes
typedef struct leventity {
	uint16_t mType;
	uint16_t mDataOfs;
} leventity_t;

// only applies to point entities, probably
typedef struct leventitydata {
	int16_t mUnknown;
	int16_t mPosition[3];
} leventitydata_t;
#pragma pack(pop)

static const signed int levcoloroffsets[18][3] = {
	{-16,-16,-16}, {-16,-15,-15}, {-15,-14,-14},
	{-14,-13,-13}, {-13,-12,-12}, {-12,-11,-11},
	{-11,-10,-10}, {-10,-9,-9}, {-9,-8,-8},
	{-8,-7,-7}, {-7,-6,-6}, {-6,-5,-5},
	{-5,-4,-4}, {-4,-3,-3}, {-3,-2,-2},
	{-2,-1,-1}, {-1,0,0}, {0,0,0}
};

static int LEV_ClusterForPoint(struct model_s *model, const vec3_t point, int *areaout)
{
	if (areaout) *areaout = 0;
	return -1;
}

static qbyte *LEV_ClusterPVS(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{
	return NULL;
}

static unsigned int LEV_FatPVS(struct model_s *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{
	return 0;
}

static qboolean LEV_EdictInFatPVS(struct model_s *model, const struct pvscache_s *edict, const qbyte *pvs, const int *areas)
{
	return true;
}

static void LEV_FindTouchedLeafs(struct model_s *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs)
{

}

static void LEV_LightPointValues(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	VectorSet(res_diffuse, 255,255,255);
	VectorSet(res_ambient, 128,128,128);
	VectorSet(res_dir, 0,0,1);
}

static qboolean match_classname(uint16_t mType, char *buf, size_t len)
{
	switch (mType)
	{
		// lights
		case 38: // static white
		case 92: // (style?)
		case 110: // heavy red pulse
		case 232: // flicker (which?)
		case 235: // (style?)
		case 253: // (style?)
			Q_strlcpy(buf, "light", len);
			return true;

		// ammo and items
		case 29: Q_strlcpy(buf, "item_shells", len); return true;
		case 88: Q_strlcpy(buf, "item_armor1", len); return true;

		// player
		case 13: Q_strlcpy(buf, "info_player_start", len); return true;

		// monsters
		case 243: Q_strlcpy(buf, "monster_army", len); return true;
		case 244: Q_strlcpy(buf, "monster_dog", len); return true;

		// decore
		case 113: Q_strlcpy(buf, "light_flame_large_yellow", len); return true;

		// unsupported
		default: return false;
	}
}

static qboolean QDECL Mod_LoadSaturnLev(model_t *mod, void *buffer, size_t fsize)
{
	int i, j, k, num_textures;
	levheader_t *header;
	levnode_t *nodes;
	levplane_t *planes;
	levtile_t *tiles;
	levvertex_t *verts;
	levquad_t *quads;
	leventity_t *ents;
	qbyte *entpolylinks;
	qbyte *entpolylinksdata1;
	qbyte *entpolylinksdata2;
	qbyte *entdata;
	qbyte *tiletex;
	qbyte *tilecol;
	qbyte *unknown1;
	galiasinfo_t *gai;
	struct bihleaf_s *bihleaf;
	int totaltris;
	int totalleafs;
	int len_entities;
	char *entities;
	char *entptr;

	// get and swap header
	header = (levheader_t *)((qbyte *)buffer + 131104);
	BigLongArray((uint32_t *)header, sizeof(*header)/sizeof(uint32_t));

	// get the rest of the offsets
	nodes = (levnode_t *)(header + 1);
	planes = (levplane_t *)(nodes + header->mNodeCount);
	tiles = (levtile_t *)(planes + header->mPlaneCount);
	verts = (levvertex_t *)(tiles + header->mTileEntryCount);
	quads = (levquad_t *)(verts + header->mVertCount);
	ents = (leventity_t *)(quads + header->mQuadCount);
	entpolylinks = (qbyte *)(ents + header->mEntityCount);
	entpolylinksdata1 = entpolylinks + 18 * header->mEntityPolyLinkCount;
	entpolylinksdata2 = entpolylinksdata1 + 2 * header->mEntityPolyLinkData1Count;
	entdata = entpolylinksdata2 + 4 * header->mEntityPolyLinkData2Count;
	tiletex = entdata + header->mEntityDataSize;
	tilecol = tiletex + header->mTileTextureDataSize;
	unknown1 = tilecol + header->mTileColorDataSize;

	// swap array data
	BigShortArray((uint16_t *)planes, (sizeof(*planes)*header->mPlaneCount)/sizeof(uint16_t));
	for (i = 0; i < header->mVertCount; i++)
		BigShortArray(verts[i].mPosition, 3);

	// figure out how many surfaces we need
	num_textures = 0;
	for (i = 0; i < header->mPlaneCount; i++)
		if (planes[i].mQuadStartIndex < header->mQuadCount)
			for (j = planes[i].mQuadStartIndex; j <= planes[i].mQuadEndIndex; j++)
				if ((int)quads[j].mTextureIndex + 1 > num_textures)
					num_textures = (int)quads[j].mTextureIndex + 1;

	if (!num_textures)
	{
		Con_Printf("%s: num_textures somehow zero!\n", mod->name);
		return false;
	}

	// bounds
	ClearBounds(mod->mins, mod->maxs);
	for (i = 0; i < header->mVertCount; i++)
	{
		vec3_t v = {
			verts[i].mPosition[0],
			verts[i].mPosition[2],
			verts[i].mPosition[1]
		};

		AddPointToBounds(v, mod->mins, mod->maxs);
	}

	// allocate one gai for each texture
	gai = ZG_Malloc(&mod->memgroup, sizeof(*gai) * num_textures);

	// count up vertices and indexes for each texture
	for (i = 0; i < header->mPlaneCount; i++)
	{
		if (planes[i].mQuadStartIndex < header->mQuadCount)
		{
			for (j = planes[i].mQuadStartIndex; j <= planes[i].mQuadEndIndex; j++)
			{
				gai[quads[j].mTextureIndex].numverts += 4;
				gai[quads[j].mTextureIndex].numindexes += 6;
			}
		}
	}

	// read in data
	totaltris = 0;
	for (i = 0; i < num_textures; i++)
	{
		int nverts, nquads;

		if (!gai[i].numverts)
		{
			Con_Printf("%s: mesh %d has invalid numverts\n", mod->name, i);
			goto next;
		}

		if (!gai[i].numindexes)
		{
			Con_Printf("%s: mesh %d has invalid numindexes\n", mod->name, i);
			goto next;
		}

		// allocate arrays
		gai[i].ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_xyz) * gai[i].numverts);
		gai[i].ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_st_array) * gai[i].numverts);
		gai[i].ofs_rgbaf = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_rgbaf) * gai[i].numverts);
		gai[i].ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_norm) * gai[i].numverts);
		gai[i].ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_svect) * gai[i].numverts);
		gai[i].ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_tvect) * gai[i].numverts);

		gai[i].ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_indexes) * gai[i].numindexes);

		// read in data
		nverts = nquads = 0;
		for (j = 0; j < header->mPlaneCount; j++)
		{
			if (planes[j].mQuadStartIndex < header->mQuadCount)
			{
				for (k = planes[j].mQuadStartIndex; k <= planes[j].mQuadEndIndex; k++)
				{
					if (quads[k].mTextureIndex == i)
					{
						int v;
						int srcverts[4];
						int dstverts[4];

						// just accumulate the verts
						for (v = 0; v < 4; v++)
						{
							srcverts[v] = planes[j].mVertStartIndex + quads[k].mIndices[v];
							dstverts[v] = nverts++;

							gai[i].ofs_skel_xyz[dstverts[v]][0] = verts[srcverts[v]].mPosition[0];
							gai[i].ofs_skel_xyz[dstverts[v]][1] = verts[srcverts[v]].mPosition[2];
							gai[i].ofs_skel_xyz[dstverts[v]][2] = verts[srcverts[v]].mPosition[1];

							gai[i].ofs_rgbaf[dstverts[v]][0] = (31.0f + (float)(levcoloroffsets[verts[srcverts[v]].mColorValue][0])) / 31.0f;
							gai[i].ofs_rgbaf[dstverts[v]][1] = (31.0f + (float)(levcoloroffsets[verts[srcverts[v]].mColorValue][1])) / 31.0f;
							gai[i].ofs_rgbaf[dstverts[v]][2] = (31.0f + (float)(levcoloroffsets[verts[srcverts[v]].mColorValue][2])) / 31.0f;
						}

						// setup tris
						gai[i].ofs_indexes[(nquads * 2 + 0) * 3 + 0] = dstverts[0];
						gai[i].ofs_indexes[(nquads * 2 + 0) * 3 + 1] = dstverts[1];
						gai[i].ofs_indexes[(nquads * 2 + 0) * 3 + 2] = dstverts[2];
						gai[i].ofs_indexes[(nquads * 2 + 1) * 3 + 0] = dstverts[0];
						gai[i].ofs_indexes[(nquads * 2 + 1) * 3 + 1] = dstverts[2];
						gai[i].ofs_indexes[(nquads * 2 + 1) * 3 + 2] = dstverts[3];
						nquads++;

						totaltris += 2;

						// setup implicit texture coords
						gai[i].ofs_st_array[dstverts[0]][0] = 0;
						gai[i].ofs_st_array[dstverts[0]][1] = 0;

						gai[i].ofs_st_array[dstverts[1]][0] = 1;
						gai[i].ofs_st_array[dstverts[1]][1] = 0;

						gai[i].ofs_st_array[dstverts[2]][0] = 1;
						gai[i].ofs_st_array[dstverts[2]][1] = 1;

						gai[i].ofs_st_array[dstverts[3]][0] = 0;
						gai[i].ofs_st_array[dstverts[3]][1] = 1;
					}
				}
			}
		}

		// generate normals and texture vectors
		Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, true);
		Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, true);

		// allocate skin
		gai[i].numskins = 1;
		gai[i].ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins) * gai[i].numskins);
		gai[i].ofsskins[0].skinspeed = 0.1;
		gai[i].ofsskins[0].numframes = 1;
		gai[i].ofsskins[0].frame = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins[0].frame));
		Q_snprintf(gai[i].ofsskins[0].name, sizeof(gai[i].ofsskins[0].name), "%s_%d", mod->name, i);
		Q_snprintf(gai[i].ofsskins[0].frame[0].shadername, sizeof(gai[i].ofsskins[0].frame[0].shadername), "%s_%d", mod->name, i);

next:
		// setup other data
		Q_snprintf(gai[i].surfacename, sizeof(gai[i].surfacename), "%s_%d", mod->name, i);
		gai[i].contents = FTECONTENTS_BODY;
		gai[i].geomset = ~0; //invalid set = always visible
		gai[i].geomid = 0;
		gai[i].mindist = 0;
		gai[i].maxdist = 0;
		gai[i].surfaceid = i;
		gai[i].shares_verts = i;
		gai[i].nextsurf = &gai[i+1];
	}

	// fix up last pointer
	gai[i-1].nextsurf = NULL;

	// allocate entities text buffer that is hopefully big enough
	len_entities = header->mEntityCount * 256;
	entities = Z_Malloc(len_entities);
	entptr = entities;

	// write worldspawn
	entptr += snprintf(entptr, len_entities - (entptr - entities), "{\"classname\"\"worldspawn\"}");

	// read entities
	for (i = 0; i < header->mEntityCount; i++)
	{
		char classname[128];
		leventitydata_t *thisentdata = (leventitydata_t *)(entdata + BigShort(ents[i].mDataOfs));

		// match classname
		if (!match_classname(BigShort(ents[i].mType), classname, sizeof(classname)))
			continue;

		entptr += snprintf(entptr, len_entities - (entptr - entities), "{\"classname\"\"%s\"", classname);

		// write known properties
		BigShortArray(thisentdata->mPosition, 3);
		entptr += snprintf(entptr, len_entities - (entptr - entities), "\"origin\"\"%d %d %d\"", thisentdata->mPosition[0], thisentdata->mPosition[2], thisentdata->mPosition[1]);

		*entptr++ = '}';
	}

	// load entities
	modfuncs->LoadEntities(mod, entities, entptr - entities);
	Z_Free(entities);

	// build BIH tree
	totalleafs = 0;
	bihleaf = Z_Malloc(sizeof(*bihleaf) * totaltris);
	for (i = 0; i < num_textures; i++)
	{
		for (j = 0; j < gai[i].numindexes; j += 3)
		{
			bihleaf[totalleafs].type = BIH_TRIANGLE;
			bihleaf[totalleafs].data.contents = FTECONTENTS_BODY;
			bihleaf[totalleafs].data.tri.indexes = &gai[i].ofs_indexes[j];
			bihleaf[totalleafs].data.tri.xyz = gai[i].ofs_skel_xyz;
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 0]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 1]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 2]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			totalleafs++;
		}
	}
	modfuncs->BIH_Build(mod, bihleaf, totaltris);
	Z_Free(bihleaf);

	// set funcs
	mod->funcs.FatPVS = LEV_FatPVS;
	mod->funcs.EdictInFatPVS = LEV_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = LEV_FindTouchedLeafs;
	mod->funcs.LightPointValues = LEV_LightPointValues;
	mod->funcs.ClusterForPoint = LEV_ClusterForPoint;
	mod->funcs.ClusterPVS = LEV_ClusterPVS;

#if 0
	// allocate gai
	gai = ZG_Malloc(&mod->memgroup, sizeof(*gai));

	// setup gai info
	gai->numverts = header->mVertCount;
	gai->numindexes = header->mQuadCount * 3 * 2;

	// allocate gai arrays
	gai->ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_xyz) * gai->numverts);
	gai->ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_st_array) * gai->numverts);
	gai->ofs_rgbaf = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_rgbaf) * gai->numverts);
	gai->ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_norm) * gai->numverts);
	gai->ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_svect) * gai->numverts);
	gai->ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_tvect) * gai->numverts);

	gai->ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_indexes) * gai->numindexes);

	// read in vertex positions and colors
	for (i = 0; i < header->mVertCount; i++)
	{
		BigShortArray(verts[i].mPosition, 3);
		gai->ofs_skel_xyz[i][0] = verts[i].mPosition[0];
		gai->ofs_skel_xyz[i][1] = verts[i].mPosition[2];
		gai->ofs_skel_xyz[i][2] = verts[i].mPosition[1];

		for (j = 0; j < 3; j++)
			gai->ofs_rgbaf[i][j] = (31.0f + (float)(levcoloroffsets[verts[i].mColorValue][j])) / 31.0f;
	}

	// read planes
	for (i = 0; i < header->mPlaneCount; i++)
	{
		if (planes[i].mQuadStartIndex < header->mQuadCount)
		{
			for (j = planes[i].mQuadStartIndex; j <= planes[i].mQuadEndIndex; j++)
			{
				int verts[4];
				for (k = 0; k < 4; k++)
					verts[k] = planes[i].mVertStartIndex + quads[j].mIndices[k];

				gai->ofs_indexes[(j * 2 + 0) * 3 + 0] = verts[0];
				gai->ofs_indexes[(j * 2 + 0) * 3 + 1] = verts[1];
				gai->ofs_indexes[(j * 2 + 0) * 3 + 2] = verts[2];
				gai->ofs_indexes[(j * 2 + 1) * 3 + 0] = verts[0];
				gai->ofs_indexes[(j * 2 + 1) * 3 + 1] = verts[2];
				gai->ofs_indexes[(j * 2 + 1) * 3 + 2] = verts[3];

				gai->ofs_st_array[verts[0]][0] = 0;
				gai->ofs_st_array[verts[0]][1] = 0;
				gai->ofs_st_array[verts[1]][0] = 1;
				gai->ofs_st_array[verts[1]][1] = 0;
				gai->ofs_st_array[verts[2]][0] = 1;
				gai->ofs_st_array[verts[2]][1] = 1;
				gai->ofs_st_array[verts[3]][0] = 0;
				gai->ofs_st_array[verts[3]][1] = 1;
			}
		}
	}

	// generate normals and texture vectors
	Mod_AccumulateTextureVectors(gai->ofs_skel_xyz, gai->ofs_st_array, gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->ofs_indexes, gai->numindexes, true);
	Mod_NormaliseTextureVectors(gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->numverts, true);

	// setup dummy texture
	gai->numskins = 1;
	gai->ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofsskins) * gai->numskins);
	gai->ofsskins->skinspeed = 0.1;
	gai->ofsskins->numframes = 1;
	gai->ofsskins->frame = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofsskins->frame));
	Q_strlcpy(gai->ofsskins->name, "default", sizeof(gai->ofsskins->name));
	Q_strlcpy(gai->ofsskins->frame[0].shadername, "default", sizeof(gai->ofsskins->frame[0].shadername));

	// setup gai
	Q_strlcpy(gai->surfacename, "default", sizeof(gai->surfacename));
	gai->contents = FTECONTENTS_BODY;
	gai->geomset = ~0; //invalid set = always visible
	gai->geomid = 0;
	gai->mindist = 0;
	gai->maxdist = 0;
	gai->surfaceid = 0;
	gai->shares_verts = 0;
#endif

	// finish up
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;
	mod->numframes = 0;
	return true;
}

qboolean Plug_Init(void)
{
	// get funcs
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs)
		return false;

	if (modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	// register format
	modfuncs->RegisterModelFormatText("Sega Saturn Quake Level (.lev)", ".lev", Mod_LoadSaturnLev);

	return true;
}
