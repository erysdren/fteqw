#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"
#include "com_bih.h"

#include "rott.h"

static plugmodfuncs_t *modfuncs;

#ifdef FTEPLUGIN
#undef ZG_Malloc
#define ZG_Malloc(g,s) plugfuncs->GMalloc(g,s)
#undef Z_Free
#define Z_Free(p) plugfuncs->Free(p)
#undef Z_Malloc
#define Z_Malloc(s) plugfuncs->Malloc(s)
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

vec_t Length(const vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}
#endif

// convert floor and ceiling textures from tile index to string
static const char *convert_updn_texture(int in)
{
	static const char *texnames[] = {
		"FLRCL1",
		"FLRCL2",
		"FLRCL3",
		"FLRCL4",
		"FLRCL5",
		"FLRCL6",
		"FLRCL7",
		"FLRCL8",
		"FLRCL9",
		"FLRCL10",
		"FLRCL11",
		"FLRCL12",
		"FLRCL13",
		"FLRCL14",
		"FLRCL15",
		"FLRCL16"
	};

	if (in >= 180 && in <= 195)
		return texnames[in - 180];
	else if (in >= 198 && in <= 213)
		return texnames[in - 198];
	else
		return NULL;
}

static qboolean is_sky_index(int in)
{
	if (in >= 234 && in <= 238)
		return true;
	else
		return false;
}

static const char *get_wall_texture(int in)
{
	static const char *walls[] = {
		"WALL1",
		"WALL2",
		"WALL3",
		"WALL4",
		"WALL5",
		"WALL6",
		"WALL7",
		"WALL8",
		"WALL9",
		"WALL10",
		"WALL11",
		"WALL12",
		"WALL13",
		"WALL14",
		"WALL15",
		"WALL16",
		"WALL17",
		"WALL18",
		"WALL19",
		"WALL20",
		"WALL21",
		"WALL22",
		"WALL23",
		"WALL24",
		"WALL25",
		"WALL26",
		"WALL27",
		"WALL28",
		"WALL29",
		"WALL30",
		"WALL31",
		"WALL32",
		"WALL33",
		"WALL34",
		"WALL35",
		"WALL36",
		"WALL37",
		"WALL38",
		"WALL39",
		"WALL40",
		"WALL41",
		"WALL42",
		"WALL43",
		"WALL44",
		"WALL45",
		"WALL46",
		"WALL47",
		"WALL48",
		"WALL49",
		"WALL50",
		"WALL51",
		"WALL52",
		"WALL53",
		"WALL54",
		"WALL55",
		"WALL56",
		"WALL57",
		"WALL58",
		"WALL59",
		"WALL60",
		"WALL61",
		"WALL62",
		"WALL63",
		"WALL64",
		"WALL65",
		"WALL66",
		"WALL67",
		"WALL68",
		"WALL69",
		"WALL70",
		"WALL71",
		"WALL72",
		"WALL73",
		"WALL74"
	};

	static const char *walbs[] = {
		"WALB03",
		"WALB04",
		"WALB05",
		"WALB06",
		"WALB07",
		"WALB08",
		"WALB13",
		"WALB14",
		"WALB15",
		"WALB16",
		"WALB17",
		"WALB18",
		"WALB19",
		"WALB20",
		"WALB21",
		"WALB26",
		"WALB27",
		"WALB28",
		"WALB40",
		"WALB41",
		"WALB42",
		"WALB44",
		"WALB46",
		"WALB48",
		"WALB51",
		"WALB58",
		"WALB59",
		"WALB60",
		"WALB61",
		"WALB69",
		"WALB70",
		"WALB71"
	};

	static const char *exits[] = {
		"EXIT",
		"ENTRANCE"
	};

	static const char *elevators[] = {
		"ELEV1",
		"ELEV2",
		"ELEV3",
		"ELEV4",
		"ELEV5",
		"ELEV6",
		"ELEV7",
		"ELEV8"
	};

	static const char *animwalls[] = {
		"FPLACE",
		"ANIMY",
		"ANIMR",
		"ANIMFAC",
		"ANIMONE",
		"ANIMTWO",
		"ANIMTHR",
		"ANIMFOR",
		"ANIMGW",
		"ANIMYOU",
		"ANIMBW",
		"ANIMBP",
		"ANIMCHN",
		"ANIMFW",
		"ANIMLAT",
		"ANIMST",
		"ANIMRP"
	};

	if ((in >= 1) && (in <= 32))
		return walls[in - 1];
	else if ((in >= 36) && (in <= 45))
		return walls[in - 3 - 1];
	else if (in == 46)
		return walls[73];
	else if ((in >= 47) && (in <= 48))
		return exits[in - 47];
	else if ((in >= 49) && (in <= 71))
		return walls[in - 8 - 1];
	else if ((in >= 72) && (in <= 79))
		return elevators[in - 72];
	else if ((in >= 80) && (in <= 89))
		return walls[in - 16 - 1];
	else if (in == 44)
		return animwalls[0];
	else if (in == 45)
		return animwalls[3];
	else if ((in >= 106) && (in <= 107))
		return animwalls[in - 105];
	else if ((in >= 224) && (in <= 233))
		return animwalls[in - 224 + 4];
	else if ((in >= 242) && (in <= 244))
		return animwalls[in - 242 + 14];
	else if ((in >= 512) && (in <= 543))
		return walbs[in - 512];
	else
		return NULL;
}

static const char *convert_sky_texture(int in)
{
	static const char *texnames[] = {
		"SKYNT1",
		"SKYDK1",
		"SKYLG1",
		"SKYLB1",
		"SKYEN1"
	};

	if (!is_sky_index(in))
		return NULL;

	return texnames[in - 234];
}

static int convert_level_brightness(int in)
{
	if (in >= 216 && in <= 223)
		return in - 216;
	return 7;
}

static size_t rlew_uncompress(uint16_t *src, size_t src_len, uint16_t tag, uint16_t *dest, size_t dest_len)
{
	size_t read, written;
	uint16_t test, rle_len, rle_value, i;

	/* read source data */
	read = 0;
	written = 0;
	while (read < src_len && written < dest_len)
	{
		/* read test value */
		test = *src++;
		read += sizeof(uint16_t);

		if (test == tag)
		{
			/* read compressed data from src */
			rle_len = *src++;
			rle_value = *src++;
			read += 2 * sizeof(uint16_t);

			/* write compressed data to dest */
			for (i = 0; i < rle_len; i++)
			{
				*dest++ = rle_value;
			}

			written += rle_len * sizeof(uint16_t);
		}
		else
		{
			/* write uncompressed data to dest */
			*dest++ = test;

			written += sizeof(uint16_t);
		}
	}

	return written;
}

static int ROTT_ClusterForPoint(struct model_s *model, const vec3_t point, int *areaout)
{
	if (areaout) *areaout = 0;
	return -1;
}

static qbyte *ROTT_ClusterPVS(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{
	return NULL;
}

static unsigned int ROTT_FatPVS(struct model_s *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{
	return 0;
}

static qboolean ROTT_EdictInFatPVS(struct model_s *model, const struct pvscache_s *edict, const qbyte *pvs, const int *areas)
{
	return true;
}

static void ROTT_FindTouchedLeafs(struct model_s *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs)
{

}

static void ROTT_LightPointValues(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	VectorSet(res_diffuse, 255,255,255);
	VectorSet(res_ambient, 128,128,128);
	VectorSet(res_dir, 0,0,1);
}

static qboolean QDECL Mod_LoadROTTModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai;
	int i, j, k;
	int len_entities;
	char *entities;
	char *entptr;
	struct bihleaf_s *bihleaf;
	int totaltris = 0;
	int totalleafs = 0;
	mrottmap_t *header = NULL;
	rottplanes_t planes;

	// sanity check
	if (memcmp(buffer, "ROTT", 4) != 0)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// get header
	header = (mrottmap_t *)buffer;

	// decompress planes
	for (i = 0; i < NUM_PLANES; i++)
		rlew_uncompress((uint16_t *)((qbyte *)buffer + header->ofs_planes[i]), header->len_planes[i], header->tag, (uint16_t *)planes[i], sizeof(planes[i]));

#if 0
	// skip preview image
	if (bufptr[3] > 2)
	{
		uint32_t w, h;
		w = *(uint32_t *)(bufptr + 4);
		h = *(uint32_t *)(bufptr + 8);
		bufptr += 12 + (w * h * 4);
	}
	else
	{
		bufptr += 4;
	}

	// get counts
	num_wads = *(uint32_t *)bufptr; bufptr += 4;
	num_entities = *(uint32_t *)bufptr; bufptr += 4;
	num_textures = *(uint32_t *)bufptr; bufptr += 4;

	// skip past wads
	for (i = 0; i < num_wads; i++)
	{
		char name[MAX_QPATH];
		strncpy(name, bufptr, sizeof(name));
		bufptr += strlen(name) + 1;
	}

	// allocate skins
	skins = ZG_Malloc(&mod->memgroup, sizeof(*skins) * num_textures);

	// read in skins
	// TODO: should add the video/ prefix for these, since the engine won't know to look there?
	for (i = 0; i < num_textures; i++)
	{
		char name[MAX_QPATH];
		strncpy(name, bufptr, sizeof(name));
		bufptr += strlen(name) + 1;

		skins[i].skinspeed = 0.1;
		skins[i].numframes = 1;
		skins[i].frame = ZG_Malloc(&mod->memgroup, sizeof(*skins[i].frame));
		Q_snprintfz(skins[i].name, sizeof(skins[i].name), "video/%s", name);
		Q_snprintfz(skins[i].frame[0].shadername, sizeof(skins[i].frame[0].shadername), "video/%s", name);
	}

	// allocate one gai for each texture
	gai = ZG_Malloc(&mod->memgroup, sizeof(*gai) * num_textures);

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// allocate entities text buffer that is hopefully big enough
	// TODO: dont be stupid, calculate this from the number of entities
	len_entities = num_entities * 256;
	entities = Z_Malloc(len_entities);
	entptr = entities;

	// read entities
	for (i = 0; i < num_entities; i++)
	{
		// read classname
		uint32_t num_properties, num_polygons;
		char classname[MAX_QPATH];
		strncpy(classname, bufptr, sizeof(classname));
		bufptr += strlen(classname) + 1;

		entptr += snprintf(entptr, len_entities - (entptr - entities), "{\"classname\"\"%s\"", classname);

		// read properties
		num_properties = *(uint32_t *)bufptr; bufptr += 4;
		for (j = 0; j < num_properties; j++)
		{
			char key[MAX_QPATH];
			char val[MAX_QPATH];
			strncpy(key, bufptr, sizeof(key));
			bufptr += strlen(key) + 1;
			strncpy(val, bufptr, sizeof(val));
			bufptr += strlen(val) + 1;

			entptr += snprintf(entptr, len_entities - (entptr - entities), "\"%s\"\"%s\"", key, val);
		}

		*entptr++ = '}';

		// read polygons
		num_polygons = *(uint32_t *)bufptr; bufptr += 4;
		if (num_polygons)
		{
			totalpolys += num_polygons;
			polygons = Z_Realloc(polygons, sizeof(*polygons) * totalpolys);
			for (j = 0; j < num_polygons; j++)
			{
				uint32_t texture;
				uint32_t num_vertices;

				polygons[totalpolys - num_polygons + j] = bufptr;

				texture = *(uint32_t *)bufptr; bufptr += 4;
				bufptr += 32; // skip plane

				// read vertices
				num_vertices = *(uint32_t *)bufptr; bufptr += 4;
				gai[texture].numverts += num_vertices;
				gai[texture].numindexes += 3 * (num_vertices - 2);
				bufptr += num_vertices * 40;
			}
		}
	}

	// load entities
	modfuncs->LoadEntities(mod, entities, entptr - entities);
	Z_Free(entities);

	// read in polygons
	for (i = 0; i < num_textures; i++)
	{
		int nverts, ntris;

		// allocate arrays
		gai[i].ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_xyz) * gai[i].numverts);
		gai[i].ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_st_array) * gai[i].numverts);
		gai[i].ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_norm) * gai[i].numverts);
		gai[i].ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_svect) * gai[i].numverts);
		gai[i].ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_tvect) * gai[i].numverts);

		gai[i].ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_indexes) * gai[i].numindexes);

		// read in our vertex positions and uvs
		nverts = ntris = 0;
		for (j = 0; j < totalpolys; j++)
		{
			uint32_t texture, num_vertices;
			qbyte *ptr = polygons[j];
			texture = *(uint32_t *)ptr; ptr += 4;
			if (texture == i)
			{
				ptr += 32; // skip plane
				num_vertices = *(uint32_t *)ptr; ptr += 4;

				// make triangle fan
				for (k = 0; k < num_vertices - 2; k++)
				{
					gai[i].ofs_indexes[ntris * 3 + 0] = nverts;
					gai[i].ofs_indexes[ntris * 3 + 1] = nverts + k + 1;
					gai[i].ofs_indexes[ntris * 3 + 2] = nverts + k + 2;
					ntris++;
					totaltris++;
				}

				// make verts
				for (k = 0; k < num_vertices; k++)
				{
					double *v = (double *)ptr; ptr += 40;
					gai[i].ofs_skel_xyz[nverts][0] = v[0];
					gai[i].ofs_skel_xyz[nverts][1] = v[2];
					gai[i].ofs_skel_xyz[nverts][2] = v[1];
					gai[i].ofs_st_array[nverts][0] = v[3];
					gai[i].ofs_st_array[nverts][1] = v[4];
					AddPointToBounds(gai[i].ofs_skel_xyz[nverts], mod->mins, mod->maxs);
					nverts++;
				}
			}
		}

		// generate normals and texture vectors
		Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, true);
		Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, true);

		// setup skin
		gai[i].numskins = 1;
		gai[i].ofsskins = &skins[i];

		// setup misc info
		Q_strlcpy(gai[i].surfacename, skins[i].name, sizeof(gai[i].surfacename));
		gai[i].contents = FTECONTENTS_BODY;
		gai[i].geomset = ~0; //invalid set = always visible
		gai[i].geomid = 0;
		gai[i].mindist = 0;
		gai[i].maxdist = 0;
		gai[i].surfaceid = i;
		gai[i].shares_verts = i;
		gai[i].nextsurf = &gai[i+1];
	}

	// reset the last pointer so it doesnt point to nothing
	gai[i-1].nextsurf = NULL;

	// build BIH tree
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
#endif

	// set funcs
	mod->funcs.FatPVS = ROTT_FatPVS;
	mod->funcs.EdictInFatPVS = ROTT_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = ROTT_FindTouchedLeafs;
	mod->funcs.LightPointValues = ROTT_LightPointValues;
	mod->funcs.ClusterForPoint = ROTT_ClusterForPoint;
	mod->funcs.ClusterPVS = ROTT_ClusterPVS;

	// finish up
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	mod->flags = 0;

	return true;
}

qboolean ROTT_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("ROTT Map File", "ROTT", 4, Mod_LoadROTTModel);
	return true;
}
