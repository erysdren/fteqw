#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"
#include "com_bih.h"

#include "rott.h"

static plugmodfuncs_t *modfuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;

static const char *rott_skies_texnames[] = {
	"SKYNT1",
	"SKYDK1",
	"SKYLG1",
	"SKYLB1",
	"SKYEN1"
};

static const char *rott_updn_texnames[] = {
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

static const char *rott_walls_texnames[] = {
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

static const char *rott_walbs_texnames[] = {
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

static const char *rott_exits_texnames[] = {
	"EXIT",
	"ENTRANCE"
};

static const char *rott_elevators_texnames[] = {
	"ELEV1",
	"ELEV2",
	"ELEV3",
	"ELEV4",
	"ELEV5",
	"ELEV6",
	"ELEV7",
	"ELEV8"
};

static const char *rott_animwalls_texnames[] = {
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

typedef struct rottmapinfo {
	q2cbrush_t *brushes;
	size_t num_brushes;
	q2cbrushside_t *brushsides;
	size_t num_brushsides;
	mplane_t *planes;
	size_t num_planes;
	q2mapsurface_t *surfaces;
	size_t num_surfaces;
	rottplanes_t mapplanes;
} rottmapinfo_t;

#ifdef FTEPLUGIN
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
	if (in >= 180 && in <= 195)
		return rott_updn_texnames[in - 180];
	else if (in >= 198 && in <= 213)
		return rott_updn_texnames[in - 198];
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
	if ((in >= 1) && (in <= 32))
		return rott_walls_texnames[in - 1];
	else if ((in >= 36) && (in <= 45))
		return rott_walls_texnames[in - 3 - 1];
	else if (in == 46)
		return rott_walls_texnames[73];
	else if ((in >= 47) && (in <= 48))
		return rott_exits_texnames[in - 47];
	else if ((in >= 49) && (in <= 71))
		return rott_walls_texnames[in - 8 - 1];
	else if ((in >= 72) && (in <= 79))
		return rott_elevators_texnames[in - 72];
	else if ((in >= 80) && (in <= 89))
		return rott_walls_texnames[in - 16 - 1];
	else if (in == 44)
		return rott_animwalls_texnames[0];
	else if (in == 45)
		return rott_animwalls_texnames[3];
	else if ((in >= 106) && (in <= 107))
		return rott_animwalls_texnames[in - 105];
	else if ((in >= 224) && (in <= 233))
		return rott_animwalls_texnames[in - 224 + 4];
	else if ((in >= 242) && (in <= 244))
		return rott_animwalls_texnames[in - 242 + 14];
	else if ((in >= 512) && (in <= 543))
		return rott_walbs_texnames[in - 512];
	else
		return NULL;
}

static int convert_level_height(int in)
{
	if (in >= 90 && in <= 97)
		return in - 89;
	else if (in >= 450 && in <= 457)
		return in - 449;
	else
		return 1;
}

static const char *convert_sky_texture(int in)
{
	if (!is_sky_index(in))
		return NULL;

	return rott_skies_texnames[in - 234];
}

#if 0
static int convert_level_brightness(int in)
{
	if (in >= 216 && in <= 223)
		return in - 216;
	return 7;
}
#endif

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

static void ROTT_GenerateMaterials(void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = (model_t *)ctx;
#if 0
	const char *script;

	if (!a)
	{	//submodels share textures, so only do this if 'a' is 0 (inline index, 0 = world).
		for(a = 0; a < mod->numtextures; a++)
		{
			script = NULL;
			if (!strncmp(mod->textures[a]->name, "sky/", 4))
				script =
				"{\n"
				"sort sky\n"
				"surfaceparm nodlight\n"
				"skyparms - - -\n"
				"}\n";
			mod->textures[a]->shader = modfuncs->RegisterBasicShader(mod, mod->textures[a]->name, SUF_LIGHTMAP, script, PTI_INVALID, 0, 0, NULL, NULL);
		}
	}
#endif
	modfuncs->Batches_Build(mod, data);
	if (data)
		plugfuncs->Free(data);
}

static void ROTT_BuildSurfMesh(model_t *mod, msurface_t *surf, builddata_t *bd)
{
#if 0
	//just builds the actual mesh data, now that it has per-batch storage allocated.
	rottmapinfo_t *prv = (rottmapinfo_t *)mod->meshinfo;
	mesh_t *mesh = surf->mesh;
	struct codsoup_s *soup = &prv->soups[surf-mod->surfaces];
	int i;

	mesh->istrifan = false;

	memcpy(mesh->xyz_array, prv->soupverts.xyz_array+soup->vertex_offset, sizeof(*mesh->xyz_array)*mesh->numvertexes);
	memcpy(mesh->normals_array, prv->soupverts.normals_array+soup->vertex_offset, sizeof(*mesh->normals_array)*mesh->numvertexes);
	for (i = 0;  i < mesh->numvertexes; i++)
		Vector4Scale(prv->soupverts.colors4b_array[soup->vertex_offset+i], 1.f/255, mesh->colors4f_array[0][i]);
	//memcpy(mesh->colors4b_array, prv->soupverts.colors4b_array+soup->vertex_offset, sizeof(*mesh->colors4b_array)*mesh->numvertexes);
	memcpy(mesh->st_array, prv->soupverts.st_array+soup->vertex_offset, sizeof(*mesh->st_array)*mesh->numvertexes);
	memcpy(mesh->lmst_array[0], prv->soupverts.lmst_array[0]+soup->vertex_offset, sizeof(*mesh->lmst_array[0])*mesh->numvertexes);

	if (soup->index_fixup)
	{
		for (i = 0; i < mesh->numindexes; i++)
			mesh->indexes[i] = prv->soupverts.indexes[soup->index_offset+i] - soup->index_fixup;
	}
	else
		memcpy(mesh->indexes, prv->soupverts.indexes+soup->index_offset, sizeof(*mesh->indexes)*mesh->numindexes);

	if (prv->soupverts.snormals_array)
	{	//cod2 made them explicit. yay.
		memcpy(mesh->snormals_array, prv->soupverts.snormals_array+soup->vertex_offset, sizeof(*mesh->snormals_array)*mesh->numvertexes);
		memcpy(mesh->tnormals_array, prv->soupverts.tnormals_array+soup->vertex_offset, sizeof(*mesh->tnormals_array)*mesh->numvertexes);
	}
	else
	{	//compute the tangents for rtlights.
		modfuncs->AccumulateTextureVectors(mesh->xyz_array, mesh->st_array, mesh->normals_array, mesh->snormals_array, mesh->tnormals_array, mesh->indexes, mesh->numindexes, false);
		modfuncs->NormaliseTextureVectors(mesh->normals_array, mesh->snormals_array, mesh->tnormals_array, mesh->numvertexes, false);
	}
#endif
}

static q2mapsurface_t *surface_for_name(rottmapinfo_t *prv, const char *texname)
{
	for (int i = 0; i < prv->num_surfaces; i++)
		if (strncmp(prv->surfaces[i].rname, texname, sizeof(prv->surfaces[i].rname)) == 0)
			return &prv->surfaces[i];
	return NULL;
}

static const vec3_t cardinal_planes[6] = {
	{1, 0, 0}, {0, 1, 0}, {0, 0, 1},
	{-1, 0, 0}, {0, -1, 0}, {0, 0, -1}
};

static void add_cube_brush(model_t *mod, rottmapinfo_t *prv, int i, int x, int y, int z, int width, int height, int depth, const char *texname)
{
	q2mapsurface_t *surface = surface_for_name(prv, texname);

	prv->brushes[i].contents = FTECONTENTS_SOLID;
	VectorSet(prv->brushes[i].absmins, x * 64, y * 64, z * 64);
	VectorSet(prv->brushes[i].absmaxs, (x * 64) + (width * 64), (y * 64) + (height * 64), (z * 64) + (depth * 64));
	prv->brushes[i].numsides = 6;
	prv->brushes[i].brushside = &prv->brushsides[i * 6];

	AddPointToBounds(prv->brushes[i].absmins, mod->mins, mod->maxs);
	AddPointToBounds(prv->brushes[i].absmaxs, mod->mins, mod->maxs);

	// setup brushsides
	for (int j = 0; j < 6; j++)
	{
		prv->brushes[i].brushside[j].plane = &prv->planes[(i * 6) + j];
		prv->brushes[i].brushside[j].surface = surface;

		if (j < 3)
			prv->brushes[i].brushside[j].plane->dist = prv->brushes[i].absmaxs[j];
		else
			prv->brushes[i].brushside[j].plane->dist = -prv->brushes[i].absmins[j - 3];

		VectorCopy(cardinal_planes[j], prv->brushes[i].brushside[j].plane->normal);

		prv->brushes[i].brushside[j].plane->type = PLANE_ANYZ;
		prv->brushes[i].brushside[j].plane->signbits = 0;
		for (int k = 0 ; k < 3 ; k++)
		{
			if (prv->brushes[i].brushside[j].plane->normal[k] < 0)
				prv->brushes[i].brushside[j].plane->signbits |= 1 << k;
			else if (prv->brushes[i].brushside[j].plane->normal[k] == 1)
				prv->brushes[i].brushside[j].plane->type = k;
		}
	}
}

static void add_surface(q2mapsurface_t **surfaces, size_t *num_surfaces, const char *texname)
{
	if (!texname)
		return;
	for (int i = 0; i < *num_surfaces; i++)
	{
		if (strncmp((*surfaces)[i].rname, texname, sizeof((*surfaces)[i].rname)) == 0)
			return;
	}

	*surfaces = plugfuncs->Realloc(*surfaces, sizeof(q2mapsurface_t) * (*num_surfaces + 1));
	(*surfaces)[*num_surfaces].c.value = 0;
	(*surfaces)[*num_surfaces].c.flags = 0;

	Q_strlcpy((*surfaces)[*num_surfaces].c.name, texname, sizeof((*surfaces)[*num_surfaces].c.name));
	Q_strlcpy((*surfaces)[*num_surfaces].rname, texname, sizeof((*surfaces)[*num_surfaces].rname));

	(*num_surfaces)++;
}

static qboolean QDECL Mod_LoadROTTModel(model_t *mod, void *buffer, size_t fsize)
{
	int i, j, k;
	int x, y, z;
	int len_entities;
	char *entities;
	char *entptr;
	struct bihleaf_s *bihleaf;
	mrottmap_t *header = NULL;
	rottmapinfo_t *prv;
	builddata_t *bd;
	q2mapsurface_t *surfaces;
	mesh_t *meshes;
	int level_floor, level_ceiling, level_height;
	int *offsets;

	// sanity check
	if (memcmp(buffer, "ROTT", 4) != 0)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// allocate prv
	prv = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv));
	mod->meshinfo = prv;

	// get header
	header = (mrottmap_t *)buffer;

	// decompress planes
	for (i = 0; i < NUM_PLANES; i++)
		rlew_uncompress((uint16_t *)((qbyte *)buffer + header->ofs_planes[i]), header->len_planes[i], header->tag, (uint16_t *)prv->mapplanes[i], sizeof(prv->mapplanes[i]));

	// count up brushes and add surfaces
	// start at 2 for floor and ceiling brushes
	prv->num_brushes = 2;
	prv->num_surfaces = 0;
	surfaces = NULL;
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			// there's a valid texture for this wall, so add a brush
			const char *texname = get_wall_texture(prv->mapplanes[0][y][x]);
			if (texname)
			{
				add_surface(&surfaces, &prv->num_surfaces, texname);
				prv->num_brushes++;
			}
		}
	}

	// setup dependent values
	prv->num_brushsides = prv->num_planes = prv->num_brushes * 6;

	// allocate brushes
	prv->brushes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->brushes) * prv->num_brushes);
	prv->planes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->planes) * prv->num_planes);
	prv->brushsides = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->brushsides) * prv->num_brushsides);

	// get tilemap height
	level_height = convert_level_height(prv->mapplanes[1][0][0]);

	// add a few more surfaces
	level_floor = prv->mapplanes[0][0][0];
	level_ceiling = prv->mapplanes[0][0][1];
	if (is_sky_index(level_ceiling))
		add_surface(&surfaces, &prv->num_surfaces, convert_sky_texture(level_ceiling));
	else
		add_surface(&surfaces, &prv->num_surfaces, convert_updn_texture(level_ceiling));
	add_surface(&surfaces, &prv->num_surfaces, convert_updn_texture(level_floor));

	// copy out surfaces
	prv->surfaces = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->surfaces) * prv->num_surfaces);
	memcpy(prv->surfaces, surfaces, sizeof(*prv->surfaces) * prv->num_surfaces);
	plugfuncs->Free(surfaces);

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// setup floor and ceiling brushes
	if (is_sky_index(level_ceiling))
		add_cube_brush(mod, prv, 1, 0, 0, level_height, MAP_WIDTH, MAP_HEIGHT, 1, convert_sky_texture(level_ceiling));
	else
		add_cube_brush(mod, prv, 1, 0, 0, level_height, MAP_WIDTH, MAP_HEIGHT, 1, convert_updn_texture(level_ceiling));
	add_cube_brush(mod, prv, 0, 0, 0, -1, MAP_WIDTH, MAP_HEIGHT, 1, convert_updn_texture(level_floor));

	// setup other brushes
	// start at 2 to skip floor and ceiling brushes
	i = 2;
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			const char *texname = get_wall_texture(prv->mapplanes[0][y][x]);
			if (!texname)
				continue;
			add_cube_brush(mod, prv, i, x, y, 0, 1, 1, level_height, texname);
			i++;
		}
	}

	// setup model surfaces
	mod->numsurfaces = prv->num_surfaces;
	mod->firstmodelsurface = 0;
	mod->nummodelsurfaces = prv->num_surfaces;
	mod->surfaces = plugfuncs->GMalloc(&mod->memgroup, (sizeof(*mod->surfaces) * prv->num_surfaces) + (sizeof(*meshes) * prv->num_surfaces));
	meshes = (mesh_t *)(mod->surfaces + prv->num_surfaces);
	for (i = 0; i < prv->num_surfaces; i++)
	{
		for (j = 0; j < MAXCPULIGHTMAPS; j++)
			mod->surfaces[i].styles[j] = INVALID_LIGHTSTYLE;

		for (j = 0; j < MAXRLIGHTMAPS; j++)
		{
			mod->surfaces[i].vlstyles[j] = INVALID_VLIGHTSTYLE;
			mod->surfaces[i].lightmaptexturenums[j] = -1;
		}

		// mod->surfaces[i].texinfo = &mod->texinfo[tex];
		mod->surfaces[i].mesh = &meshes[i];
		mod->surfaces[i].mesh->numvertexes = 0;
		mod->surfaces[i].mesh->numindexes = 0;
	}

	// accumulate vertices and indexes for each wall tile
	// FIXME: doesn't prune hidden or parallel faces
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			q2mapsurface_t *mapsurface;
			msurface_t *surface;
			const char *texname = get_wall_texture(prv->mapplanes[0][y][x]);
			if (!texname)
				continue;
			mapsurface = surface_for_name(prv, texname);
			surface = &mod->surfaces[mapsurface - prv->surfaces];
			surface->mesh->numvertexes += 8;
			surface->mesh->numindexes += 24;
		}
	}

	// allocate mesh arrays
	for (i = 0; i < prv->num_surfaces; i++)
	{
		meshes[i].xyz_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].xyz_array) * meshes[i].numvertexes);
		meshes[i].st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].st_array) * meshes[i].numvertexes);
		meshes[i].normals_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].normals_array) * meshes[i].numvertexes);
		meshes[i].snormals_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].snormals_array) * meshes[i].numvertexes);
		meshes[i].tnormals_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].tnormals_array) * meshes[i].numvertexes);

		meshes[i].indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*meshes[i].indexes) * meshes[i].numindexes);
	}

	// read in mesh data
	offsets = plugfuncs->Malloc(sizeof(*offsets) * prv->num_surfaces);
	memset(offsets, 0, sizeof(*offsets) * prv->num_surfaces);
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			int v[8];
			int s;
			const char *texname = get_wall_texture(prv->mapplanes[0][y][x]);
			if (!texname)
				continue;
			s = surface_for_name(prv, texname) - prv->surfaces;
			/*
			 * bottom    top
			 * 0 -- 1    4 -- 5
			 * |    |    |    |
			 * |    |    |    |
			 * 3 -- 2    7 -- 6
			 *
			 * north     south
			 * 5 -- 4    7 -- 6
			 * | \ B|    | \ B|
			 * |A \ |    |A \ |
			 * 1 -- 0    3 -- 2
			 *
			 * east      west
			 * 6 -- 5    4 -- 7
			 * | \ B|    | \ B|
			 * |A \ |    |A \ |
			 * 2 -- 1    0 -- 3
			 */
			// setup vertices
			for (i = 0; i < 8; i++)
			{
				// positions
				if (i == 0 || i == 3 || i == 4 || i == 7)
					meshes[s].xyz_array[offsets[s] * 8 + i][0] = (x * 64);
				else
					meshes[s].xyz_array[offsets[s] * 8 + i][0] = (x * 64) + 64;
				if (i == 0 || i == 1 || i == 4 || i == 5)
					meshes[s].xyz_array[offsets[s] * 8 + i][1] = (y * 64);
				else
					meshes[s].xyz_array[offsets[s] * 8 + i][1] = (y * 64) + 64;
				if (i < 4)
					meshes[s].xyz_array[offsets[s] * 8 + i][2] = 0;
				else
					meshes[s].xyz_array[offsets[s] * 8 + i][2] = level_height * 64;
				// texcoords
				if (i == 0 || i == 2 || i == 4 || i == 6)
					meshes[s].st_array[offsets[s] * 8 + i][0] = 0;
				else
					meshes[s].st_array[offsets[s] * 8 + i][0] = 1;
				if (i < 4)
					meshes[s].st_array[offsets[s] * 8 + i][1] = 1;
				else
					meshes[s].st_array[offsets[s] * 8 + i][1] = 0;
				// save indices
				v[i] = offsets[s] * 8 + i;
			}
			// setup faces
			for (i = 0; i < 4; i++)
			{
				int t[6];

				switch (i)
				{
					// north
					case 0:
						t[0] = v[5];
						t[1] = v[1];
						t[2] = v[0];

						t[3] = v[5];
						t[4] = v[0];
						t[5] = v[4];
						break;

					// east
					case 1:
						t[0] = v[6];
						t[1] = v[2];
						t[2] = v[1];

						t[3] = v[6];
						t[4] = v[1];
						t[5] = v[5];
						break;

					// south
					case 2:
						t[0] = v[7];
						t[1] = v[3];
						t[2] = v[2];

						t[3] = v[7];
						t[4] = v[2];
						t[5] = v[6];
						break;

					// west
					case 3:
						t[0] = v[4];
						t[1] = v[0];
						t[2] = v[3];

						t[3] = v[4];
						t[4] = v[3];
						t[5] = v[7];
						break;
				}

				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 0] = t[0];
				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 1] = t[1];
				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 2] = t[2];

				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 3] = t[3];
				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 4] = t[4];
				meshes[s].indexes[offsets[s] * 24 + (i * 6) + 5] = t[5];
			}
			offsets[s]++;
		}
	}
	plugfuncs->Free(offsets);

	// compute normals and tangents
	for (i = 0; i < prv->num_surfaces; i++)
	{
		modfuncs->AccumulateTextureVectors(meshes[i].xyz_array, meshes[i].st_array, meshes[i].normals_array, meshes[i].snormals_array, meshes[i].tnormals_array, meshes[i].indexes, meshes[i].numindexes, true);
		modfuncs->NormaliseTextureVectors(meshes[i].normals_array, meshes[i].snormals_array, meshes[i].tnormals_array, meshes[i].numvertexes, true);
	}

	// build BIH tree
	bihleaf = plugfuncs->Malloc(sizeof(*bihleaf) * prv->num_brushes);
	for (i = 0; i < prv->num_brushes; i++)
	{
		bihleaf[i].type = BIH_BRUSH;
		bihleaf[i].data.contents = FTECONTENTS_SOLID;
		bihleaf[i].data.brush = &prv->brushes[i];
	}
	modfuncs->BIH_Build(mod, bihleaf, prv->num_brushes);
	plugfuncs->Free(bihleaf);

	// build batches
	mod->hulls[0].firstclipnode = -1;
	mod->nodes = mod->rootnode = NULL;
	mod->leafs = NULL;
	memset(&mod->batches, 0, sizeof(mod->batches));
	mod->vbos = NULL;
#ifdef HAVE_CLIENT
	bd = plugfuncs->Malloc(sizeof(*bd));
	bd->buildfunc = ROTT_BuildSurfMesh;
	bd->paintlightmaps = true;
	threadfuncs->AddWork(WG_MAIN, ROTT_GenerateMaterials, mod, bd, 0, 0);
#endif

	// add entities
	len_entities = 8192;
	entptr = entities = plugfuncs->Malloc(len_entities);
	Q_snprintfz(entities, len_entities, "{\n\"classname\"\"worldspawn\"\n}\n{\n\"classname\"\"info_player_start\"\"origin\"\"0 0 32\"\n}\n");
	modfuncs->LoadEntities(mod, entities, len_entities);
	plugfuncs->Free(entities);

	// set funcs
	mod->funcs.FatPVS = ROTT_FatPVS;
	mod->funcs.EdictInFatPVS = ROTT_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = ROTT_FindTouchedLeafs;
	mod->funcs.LightPointValues = ROTT_LightPointValues;
	mod->funcs.ClusterForPoint = ROTT_ClusterForPoint;
	mod->funcs.ClusterPVS = ROTT_ClusterPVS;

	// finish up
	mod->type = mod_brush;
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	mod->flags = 0;

	return true;

#if 0
	// allocate floor gai
	gai = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai) * 2);
	gai->numverts = 4;
	gai->numindexes = 6;

	// allocate floor arrays
	gai->ofs_skel_xyz = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_xyz) * gai->numverts);
	gai->ofs_st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_st_array) * gai->numverts);
	gai->ofs_skel_norm = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_norm) * gai->numverts);
	gai->ofs_skel_svect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_svect) * gai->numverts);
	gai->ofs_skel_tvect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_tvect) * gai->numverts);

	gai->ofs_indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_indexes) * gai->numindexes);

	// setup floor triangles
	gai->ofs_indexes[0 * 3 + 0] = 2;
	gai->ofs_indexes[0 * 3 + 1] = 1;
	gai->ofs_indexes[0 * 3 + 2] = 0;
	gai->ofs_indexes[1 * 3 + 0] = 3;
	gai->ofs_indexes[1 * 3 + 1] = 2;
	gai->ofs_indexes[1 * 3 + 2] = 0;

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// setup floor vertices
	for (i = 0; i < 4; i++)
	{
		switch (i)
		{
			case 0:
				gai->ofs_skel_xyz[i][0] = 0;
				gai->ofs_skel_xyz[i][1] = 0;
				gai->ofs_st_array[i][0] = 0;
				gai->ofs_st_array[i][1] = 0;
				break;
			case 1:
				gai->ofs_skel_xyz[i][0] = MAP_WIDTH * 64;
				gai->ofs_skel_xyz[i][1] = 0;
				gai->ofs_st_array[i][0] = 64;
				gai->ofs_st_array[i][1] = 0;
				break;
			case 2:
				gai->ofs_skel_xyz[i][0] = MAP_WIDTH * 64;
				gai->ofs_skel_xyz[i][1] = MAP_HEIGHT * 64;
				gai->ofs_st_array[i][0] = 64;
				gai->ofs_st_array[i][1] = 64;
				break;
			case 3:
				gai->ofs_skel_xyz[i][0] = 0;
				gai->ofs_skel_xyz[i][1] = MAP_HEIGHT * 64;
				gai->ofs_st_array[i][0] = 0;
				gai->ofs_st_array[i][1] = 64;
				break;
		}

		gai->ofs_skel_xyz[i][2] = 0;

		AddPointToBounds(gai->ofs_skel_xyz[i], mod->mins, mod->maxs);
	}

	// allocate floor skin
	gai->numskins = 1;
	gai->ofsskins = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofsskins) * gai->numskins);
	gai->ofsskins[0].skinspeed = 0.1;
	gai->ofsskins[0].numframes = 1;
	gai->ofsskins[0].frame = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofsskins[0].frame));
	Q_strlcpy(gai->ofsskins[0].name, convert_updn_texture(planes[0][0][0]), sizeof(gai->ofsskins[i].name));
	Q_strlcpy(gai->ofsskins[0].frame[0].shadername, convert_updn_texture(planes[0][0][0]), sizeof(gai->ofsskins[0].frame[0].shadername));

	// setup misc info
	Q_strlcpy(gai->surfacename, convert_updn_texture(planes[0][0][0]), sizeof(gai->surfacename));
	gai->contents = FTECONTENTS_SOLID;
	gai->geomset = ~0; //invalid set = always visible
	gai->geomid = 0;
	gai->mindist = 0;
	gai->maxdist = 0;
	gai->surfaceid = 0;
	gai->shares_verts = 0;
	//gai->nextsurf = &gai[1];

	// generate normals and texture vectors
	modfuncs->AccumulateTextureVectors(gai->ofs_skel_xyz, gai->ofs_st_array, gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->ofs_indexes, gai->numindexes, true);
	modfuncs->NormaliseTextureVectors(gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->numverts, true);

	// build brushes
	brushes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*brushes) * totalbrushes);
	brushsides = plugfuncs->GMalloc(&mod->memgroup, sizeof(*brushsides) * totalbrushes * 6);
	brushsurfaces = plugfuncs->GMalloc(&mod->memgroup, sizeof(*brushsurfaces) * totalbrushes * 6);
	brushplanes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*brushplanes) * totalbrushes * 6);
	for (i = 0; i < totalbrushes; i++)
	{
		brushes[i].contents = FTECONTENTS_SOLID;
		VectorSet(brushes[i].absmins, 0, 0, -64);
		VectorSet(brushes[i].absmaxs, MAP_WIDTH * 64, MAP_HEIGHT * 64, 0);
		brushes[i].numsides = 6;
		brushes[i].brushside = &brushsides[i * 6];
		for (j = 0; j < 6; j++)
		{
			brushes[i].brushside[j].surface = &brushsurfaces[i * 6 + j];
			brushes[i].brushside[j].plane = &brushplanes[i * 6 + j];
		}

		// copy cardinal planes
		VectorCopy(cardinal_planes[0], brushes[i].brushside[0].plane->normal);
		VectorCopy(cardinal_planes[1], brushes[i].brushside[1].plane->normal);
		VectorCopy(cardinal_planes[2], brushes[i].brushside[2].plane->normal);
		VectorCopy(cardinal_planes[3], brushes[i].brushside[3].plane->normal);
		VectorCopy(cardinal_planes[4], brushes[i].brushside[4].plane->normal);
		VectorCopy(cardinal_planes[5], brushes[i].brushside[5].plane->normal);

		// copy plane distances
		brushes[i].brushside[0].plane->dist = brushes[i].absmaxs[0];
		brushes[i].brushside[1].plane->dist = brushes[i].absmaxs[1];
		brushes[i].brushside[2].plane->dist = brushes[i].absmaxs[2];
		brushes[i].brushside[3].plane->dist = -brushes[i].absmins[0];
		brushes[i].brushside[4].plane->dist = -brushes[i].absmins[1];
		brushes[i].brushside[5].plane->dist = -brushes[i].absmins[2];

		// setup plane types and signbits
		for (j = 0; j < 6; j++)
		{
			brushes[i].brushside[j].plane->type = PLANE_ANYZ;
			brushes[i].brushside[j].plane->signbits = 0;
			for (k = 0 ; k < 3 ; k++)
			{
				if (brushes[i].brushside[j].plane->normal[k] < 0)
					brushes[i].brushside[j].plane->signbits |= 1 << k;
				else if (brushes[i].brushside[j].plane->normal[k] == 1)
					brushes[i].brushside[j].plane->type = k;
			}
		}
	}

	// build BIH tree
	bihleaf = plugfuncs->Malloc(sizeof(*bihleaf) * totalbrushes);
	for (i = 0; i < totalbrushes; i++)
	{
		bihleaf[i].type = BIH_BRUSH;
		bihleaf[i].data.contents = FTECONTENTS_SOLID;
		bihleaf[i].data.brush = &brushes[i];
	}
	modfuncs->BIH_Build(mod, bihleaf, totalbrushes);
	plugfuncs->Free(bihleaf);

	// add entities
	entities = plugfuncs->Malloc(8192);
	Q_snprintfz(entities, 8192, "{\n\"classname\"\"worldspawn\"\n}\n{\n\"classname\"\"info_player_start\"\"origin\"\"0 0 64\"\n}\n");
	modfuncs->LoadEntities(mod, entities, 8192);
	plugfuncs->Free(entities);
#endif

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

#if 0
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
#endif
}

qboolean ModROTT_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	if (!threadfuncs || !modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("ROTT Map File", "ROTT", 4, Mod_LoadROTTModel);
	return true;
}
