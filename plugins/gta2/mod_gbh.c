/*
 * written by erysdren (it/its) in 2025
 *
 * based on the official format spec
 * https://archive.org/download/gta2-1999/Files/Formats%20Spec/gta2formats.zip/GTA2%20Map%20Format.doc
 */
#include "../plugin.h"
#include "../engine/common/com_mesh.h"
#include "../engine/common/com_bih.h"

#include "sty.h"

#ifdef FTEPLUGIN
#undef ZG_Malloc
#define ZG_Malloc(g,s) plugfuncs->GMalloc(g,s)
#undef Z_Free
#define Z_Free(p) plugfuncs->Free(p)
#undef Z_Malloc
#define Z_Malloc(s) plugfuncs->Free(s)
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

static plugthreadfuncs_t *threadfuncs = NULL;

enum {
	GROUND_TYPE_AIR = 0,
	GROUND_TYPE_ROAD = 1,
	GROUND_TYPE_PAVEMENT = 2,
	GROUND_TYPE_FIELD = 3
};

enum {
	SLOPE_TYPE_NONE = 0,
	SLOPE_TYPE_UP26_0 = 1,
	SLOPE_TYPE_UP26_1 = 2,
	SLOPE_TYPE_DOWN26_0 = 3,
	SLOPE_TYPE_DOWN26_1 = 4,
	SLOPE_TYPE_LEFT26_0 = 5,
	SLOPE_TYPE_LEFT26_1 = 6,
	SLOPE_TYPE_RIGHT26_0 = 7,
	SLOPE_TYPE_RIGHT26_1 = 8,
	SLOPE_TYPE_UP7_0 = 9,
	SLOPE_TYPE_UP7_1 = 10,
	SLOPE_TYPE_UP7_2 = 11,
	SLOPE_TYPE_UP7_3 = 12,
	SLOPE_TYPE_UP7_4 = 13,
	SLOPE_TYPE_UP7_5 = 14,
	SLOPE_TYPE_UP7_6 = 15,
	SLOPE_TYPE_UP7_7 = 16,
	SLOPE_TYPE_DOWN7_0 = 17,
	SLOPE_TYPE_DOWN7_1 = 18,
	SLOPE_TYPE_DOWN7_2 = 19,
	SLOPE_TYPE_DOWN7_3 = 20,
	SLOPE_TYPE_DOWN7_4 = 21,
	SLOPE_TYPE_DOWN7_5 = 22,
	SLOPE_TYPE_DOWN7_6 = 23,
	SLOPE_TYPE_DOWN7_7 = 24,
	SLOPE_TYPE_LEFT7_0 = 25,
	SLOPE_TYPE_LEFT7_1 = 26,
	SLOPE_TYPE_LEFT7_2 = 27,
	SLOPE_TYPE_LEFT7_3 = 28,
	SLOPE_TYPE_LEFT7_4 = 29,
	SLOPE_TYPE_LEFT7_5 = 30,
	SLOPE_TYPE_LEFT7_6 = 31,
	SLOPE_TYPE_LEFT7_7 = 32,
	SLOPE_TYPE_RIGHT7_0 = 33,
	SLOPE_TYPE_RIGHT7_1 = 34,
	SLOPE_TYPE_RIGHT7_2 = 35,
	SLOPE_TYPE_RIGHT7_3 = 36,
	SLOPE_TYPE_RIGHT7_4 = 37,
	SLOPE_TYPE_RIGHT7_5 = 38,
	SLOPE_TYPE_RIGHT7_6 = 39,
	SLOPE_TYPE_RIGHT7_7 = 40,
	SLOPE_TYPE_45UP = 41,
	SLOPE_TYPE_45DOWN = 42,
	SLOPE_TYPE_45LEFT = 43,
	SLOPE_TYPE_45RIGHT = 44,
	SLOPE_TYPE_DIAGONALFACINGUPLEFT = 45,
	SLOPE_TYPE_DIAGONALFACINGUPRIGHT = 46,
	SLOPE_TYPE_DIAGONALFACINGDOWNLEFT = 47,
	SLOPE_TYPE_DIAGONALFACINGDOWNRIGHT = 48,
	SLOPE_TYPE_DIAGONALSLOPEFACINGUPLEFT = 49,
	SLOPE_TYPE_DIAGONALSLOPEFACINGUPRIGHT = 50,
	SLOPE_TYPE_DIAGONALSLOPEFACINGDOWNLEFT = 51,
	SLOPE_TYPE_DIAGONALSLOPEFACINGDOWNRIGHT = 52,
	SLOPE_TYPE_PARTIALBLOCKLEFT = 53,
	SLOPE_TYPE_PARTIALBLOCKRIGHT = 54,
	SLOPE_TYPE_PARTIALBLOCKTOP = 55,
	SLOPE_TYPE_PARTIALBLOCKBOTTOM = 56,
	SLOPE_TYPE_PARTIALBLOCKTOPLEFTCORNER = 57,
	SLOPE_TYPE_PARTIALBLOCKTOPRIGHTCORNER = 58,
	SLOPE_TYPE_PARTIALBLOCKBOTTOMRIGHTCORNER = 59,
	SLOPE_TYPE_PARTIALBLOCKBOTTOMLEFTCORNER = 60,
	SLOPE_TYPE_PARTIALCENTERBLOCK = 61,
	SLOPE_TYPE_RESERVED = 62,
	SLOPE_TYPE_SLOPEABOVE = 63
};

#define GROUND_TYPE(n) ((n) & 0x3)
#define SLOPE_TYPE(n) (((n) >> 2) & 0x3F)
#define WALL_GRAPHIC(n) ((n) & 0x3FF)
#define WALL_COLLIDE(n) (((n) >> 9) & 0x1)
#define WALL_BULLET(n) (((n) >> 10) & 0x1)
#define WALL_FLAT(n) (((n) >> 11) & 0x1)
#define WALL_FLIP(n) (((n) >> 12) & 0x1)
#define WALL_ROTATION(n) (((n) >> 13) & 0x3)

#define LID_GRAPHIC(n) ((n) & 0x3FF)
#define LID_LIGHTING(n) (((n) >> 9) & 0x3)
#define LID_FLAT(n) (((n) >> 11) & 0x1)
#define LID_FLIP(n) (((n) >> 12) & 0x1)
#define LID_ROTATION(n) (((n) >> 13) & 0x3)

#define CITY_OFFSET_X (128 * 64)
#define CITY_OFFSET_Y (128 * 64)

static plugmodfuncs_t *modfuncs = NULL;

typedef struct gbhblockinfo {
	uint16_t left, right, top, bottom, lid;
	uint8_t arrows;
	uint8_t slope_type;
} gbhblockinfo_t;

typedef struct gbhcolumninfo {
	uint8_t height;
	uint8_t offset;
} gbhcolumninfo_t;

typedef struct gbhheader {
	uint32_t magic;
	uint16_t version;
} gbhheader_t;

typedef struct gbhchunk {
	uint32_t magic;
	uint32_t size;
} gbhchunk_t;

typedef gbhblockinfo_t gbhcity_t[8][256][256];

static gbhchunk_t *GBH_FindChunk(const char *type, uint8_t *buffer, size_t fsize)
{
	uint8_t *ptr = buffer;
	uint8_t *end = buffer + fsize;

	while (ptr < end)
	{
		gbhchunk_t *chunk = (gbhchunk_t *)ptr;
		if (memcmp(&chunk->magic, type, 4) == 0)
			return chunk;
		ptr += sizeof(gbhchunk_t) + chunk->size;
	}

	return NULL;
}

static qboolean GBH_LoadCMAP(model_t *mod, gbhchunk_t *chunk, gbhcity_t city)
{
	uint16_t *base;
	uint16_t column_words;
	uint16_t *column;
	uint16_t num_blocks;
	gbhblockinfo_t *blocks;
	int x, y;

	// sanity check
	if (chunk->size < 256 * 256 * sizeof(uint16_t))
	{
		Con_Printf("%s: CMAP chunk size mismatch\n", mod->name);
		return false;
	}

	base = (uint16_t *)(chunk + 1);
	column_words = *(base + (256 * 256));
	column = base + (256 * 256) + 1;
	num_blocks = *(column + column_words);
	blocks = (gbhblockinfo_t *)(column + column_words + 1);

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			int i;
			uint8_t height, offset;
			uint16_t *blockd;
			uint16_t col = base[y * 256 + x];

			// sanity check
			if (col >= column_words)
			{
				Con_Printf("%s: column %d out of range\n", mod->name, col);
				return false;
			}

			height = ((uint8_t *)&column[col])[0];
			offset = ((uint8_t *)&column[col])[1];
			blockd = &column[col] + 1;

			// sanity check
			if (height > 7)
			{
				Con_Printf("%s: height %d out of range\n", mod->name, height);
				return false;
			}

			// sanity check
			if (offset > height)
			{
				Con_Printf("%s: offset %d out of range\n", mod->name, offset);
				return false;
			}

			// copy blocks
			for (i = 0; i < height; i++)
			{
				if (blockd[i] >= num_blocks)
				{
					Con_Printf("%s: block %d out of range\n", mod->name, blockd[i]);
					return false;
				}

				if (i >= offset)
				{
					memcpy(&city[i][y][x], &blocks[blockd[i - offset]], sizeof(gbhblockinfo_t));
				}
			}
		}
	}

	return true;
}

static qboolean GBH_LoadDMAP(model_t *mod, gbhchunk_t *chunk, gbhcity_t city)
{
	uint32_t *base;
	uint32_t column_words;
	uint32_t *column;
	uint32_t num_blocks;
	gbhblockinfo_t *blocks;
	int x, y;

	// sanity check
	if (chunk->size < 256 * 256 * sizeof(uint32_t))
	{
		Con_Printf("%s: DMAP chunk size mismatch\n", mod->name);
		return false;
	}

	base = (uint32_t *)(chunk + 1);
	column_words = *(base + (256 * 256));
	column = base + (256 * 256) + 1;
	num_blocks = *(column + column_words);
	blocks = (gbhblockinfo_t *)(column + column_words + 1);

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			int i;
			uint8_t height, offset;
			uint32_t *blockd;
			uint32_t col = base[y * 256 + x];

			// sanity check
			if (col >= column_words)
			{
				Con_Printf("%s: column %d out of range\n", mod->name, col);
				return false;
			}

			height = ((uint8_t *)&column[col])[0];
			offset = ((uint8_t *)&column[col])[1];
			blockd = &column[col] + 1;

			// sanity check
			if (height > 7)
			{
				Con_Printf("%s: height %d out of range\n", mod->name, height);
				return false;
			}

			// sanity check
			if (offset > height)
			{
				Con_Printf("%s: offset %d out of range\n", mod->name, offset);
				return false;
			}

			// copy blocks
			for (i = 0; i < height; i++)
			{
				if (blockd[i] >= num_blocks)
				{
					Con_Printf("%s: block %d out of range\n", mod->name, blockd[i]);
					return false;
				}

				if (i >= offset)
				{
					memcpy(&city[i][y][x], &blocks[blockd[i - offset]], sizeof(gbhblockinfo_t));
				}
			}
		}
	}

	return true;
}

static qboolean GBH_LoadUMAP(model_t *mod, gbhchunk_t *chunk, gbhcity_t city)
{
	// sanity check
	if (chunk->size < sizeof(gbhcity_t))
	{
		Con_Printf("%s: UMAP chunk size mismatch\n", mod->name);
		return false;
	}

	memcpy(city, (void *)(chunk + 1), sizeof(gbhcity_t));

	return true;
}

static void GBH_GenerateMaterials(void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = (model_t *)ctx;
	sty_t *sty = (sty_t *)data;
	int i;

	mod->numtextures = STY_NUM_TILES;
	mod->textures = plugfuncs->GMalloc(&mod->memgroup, sizeof(texture_t *) * mod->numtextures);
	for (i = 0; i < mod->numtextures; i++)
	{
		Q_snprintf(mod->textures[a]->name, sizeof(mod->textures[a]->name), "%stile%d", mod->name, i);
		mod->textures[a]->shader = modfuncs->RegisterBasicShader(mod, mod->textures[a]->name, SUF_NONE, NULL, TF_8PAL32, STY_TILE_WIDTH, STY_TILE_HEIGHT, sty->tiles[1], sty->palettes[1]);
	}

	plugfuncs->Free(sty);
}

static qboolean QDECL Mod_LoadGBH(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai = NULL;
	gbhheader_t *header = NULL;
	gbhchunk_t *chunk = NULL;
	uint8_t *chunkstart = NULL;
	size_t chunksize = 0;
	qboolean loaded = false;
	int x, y, z;
	int i, j, k;
	int nquads;
	gbhcity_t *city;
	sty_t *sty;
	char styfilename[MAX_OSPATH];

	header = (gbhheader_t *)buffer;

	if (memcmp(&header->magic, "GBMP", 4) != 0)
	{
		Con_Printf("%s: unknown format\n", mod->name);
		return false;
	}

	if (header->version != 500)
	{
		Con_Printf("%s: unknown version %d\n", mod->name, header->version);
		return false;
	}

	// allocate city
	city = plugfuncs->Malloc(sizeof(*city));
	memset(city, 0, sizeof(*city));

	chunkstart = (uint8_t *)buffer + 6;
	chunksize = fsize - 6;

	// prefer uncompressed map, less work for us
	chunk = GBH_FindChunk("UMAP", chunkstart, chunksize);
	if (chunk)
		loaded = GBH_LoadUMAP(mod, chunk, *city);

	// try DMAP (32-bit compressed map)
	if (!loaded)
	{
		chunk = GBH_FindChunk("DMAP", chunkstart, chunksize);
		if (chunk)
			loaded = GBH_LoadDMAP(mod, chunk, *city);
	}

	// try CMAP (16-bit compressed map)
	if (!loaded)
	{
		chunk = GBH_FindChunk("CMAP", chunkstart, chunksize);
		if (chunk)
			loaded = GBH_LoadCMAP(mod, chunk, *city);
	}

	// failed
	if (!loaded)
	{
		Con_Printf("%s: failed to load map data\n", mod->name);
		return false;
	}

	// bounds
	ClearBounds(mod->mins, mod->maxs);

	// load sty
	modfuncs->StripExtension(mod->name, styfilename, sizeof(styfilename));
	Q_strlcat(styfilename, ".sty", sizeof(styfilename));
	sty = GTA2_LoadSty(styfilename);
	if (!sty)
		Con_Printf("%s: failed to load associated style file \"%s\"\n", mod->name, styfilename);

	// generate materials
	// NOTE: this also frees the sty buffer
	if (sty)
		threadfuncs->AddWork(WG_MAIN, GBH_GenerateMaterials, mod, sty, 0, 0);

	// generate geometry from map
	gai = ZG_Malloc(&mod->memgroup, sizeof(*gai));

	// is there a way to speed this up?
	for (z = 0; z < 8; z++)
	{
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				if (GROUND_TYPE((*city)[z][y][x].slope_type) == GROUND_TYPE_ROAD)
				{
					gai[0].numverts += 4;
					gai[0].numindexes += 6;
				}
			}
		}
	}

	// allocate arrays
	gai[0].ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_xyz) * gai[0].numverts);
	gai[0].ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_st_array) * gai[0].numverts);
	gai[0].ofs_rgbaf = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_rgbaf) * gai[0].numverts);
	gai[0].ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_norm) * gai[0].numverts);
	gai[0].ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_svect) * gai[0].numverts);
	gai[0].ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_tvect) * gai[0].numverts);

	gai[0].ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofs_indexes) * gai[0].numindexes);

	// generate vertex positions and indices
	nquads = 0;
	for (z = 0; z < 8; z++)
	{
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				if (GROUND_TYPE((*city)[z][y][x].slope_type) == GROUND_TYPE_ROAD)
				{
					int dstverts[4] = {
						(nquads * 4) + 0,
						(nquads * 4) + 1,
						(nquads * 4) + 2,
						(nquads * 4) + 3
					};

					// setup positions
					gai[0].ofs_skel_xyz[dstverts[0]][0] = (x * 64) - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[0]][1] = (y * 64) - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[0]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[1]][0] = (x * 64) - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[1]][1] = (y * 64) + 64 - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[1]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[2]][0] = (x * 64) + 64 - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[2]][1] = (y * 64) + 64 - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[2]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[3]][0] = (x * 64) + 64 - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[3]][1] = (y * 64) - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[3]][2] = (z * 64);

					// bounds
					for (i = 0; i < 4; i++)
						AddPointToBounds(gai[0].ofs_skel_xyz[dstverts[i]], mod->mins, mod->maxs);

					// setup indices
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 0] = dstverts[0];
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 1] = dstverts[1];
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 2] = dstverts[2];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 0] = dstverts[0];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 1] = dstverts[2];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 2] = dstverts[3];

					// setup texture coords
					gai[0].ofs_st_array[dstverts[0]][0] = 0;
					gai[0].ofs_st_array[dstverts[0]][1] = 0;

					gai[0].ofs_st_array[dstverts[1]][0] = 1;
					gai[0].ofs_st_array[dstverts[1]][1] = 0;

					gai[0].ofs_st_array[dstverts[2]][0] = 1;
					gai[0].ofs_st_array[dstverts[2]][1] = 1;

					gai[0].ofs_st_array[dstverts[3]][0] = 0;
					gai[0].ofs_st_array[dstverts[3]][1] = 1;

					nquads++;
				}
			}
		}
	}

	// generate normals and texture vectors
	Mod_AccumulateTextureVectors(gai[0].ofs_skel_xyz, gai[0].ofs_st_array, gai[0].ofs_skel_norm, gai[0].ofs_skel_svect, gai[0].ofs_skel_tvect, gai[0].ofs_indexes, gai[0].numindexes, true);
	Mod_NormaliseTextureVectors(gai[0].ofs_skel_norm, gai[0].ofs_skel_svect, gai[0].ofs_skel_tvect, gai[0].numverts, true);

	// allocate skin
	gai[0].numskins = 1;
	gai[0].ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofsskins) * gai[0].numskins);
	gai[0].ofsskins[0].skinspeed = 0.1;
	gai[0].ofsskins[0].numframes = 1;
	gai[0].ofsskins[0].frame = ZG_Malloc(&mod->memgroup, sizeof(*gai[0].ofsskins[0].frame));
	Q_snprintf(gai[0].ofsskins[0].name, sizeof(gai[0].ofsskins[0].name), "default");
	Q_snprintf(gai[0].ofsskins[0].frame[0].shadername, sizeof(gai[0].ofsskins[0].frame[0].shadername), "default");

	// setup other data
	Q_snprintf(gai[0].surfacename, sizeof(gai[0].surfacename), "default");
	gai[0].contents = FTECONTENTS_BODY;
	gai[0].geomset = ~0; //invalid set = always visible
	gai[0].geomid = 0;
	gai[0].mindist = 0;
	gai[0].maxdist = 0;
	gai[0].surfaceid = 0;
	gai[0].shares_verts = 0;
	gai[0].nextsurf = NULL;

	// cleanup
	plugfuncs->Free(city);

	// finish setting up model
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;
	mod->numframes = 0;
	return true;
}

qboolean Plug_GBH_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	if (!modfuncs || !threadfuncs)
		return false;

	if (modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Grand Theft Auto 2 Map (.gmp)", "GBMP", 4, Mod_LoadGBH);

	return true;
}
