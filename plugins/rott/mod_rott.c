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

typedef struct rottmaparea {
	q2mapsurface_t *surfaces;
	size_t num_surfaces;
	mesh_t *meshes;
	size_t num_meshes;
} rottmaparea_t;

typedef struct rottmapinfo {
	q2cbrush_t *brushes;
	size_t num_brushes;
	q2cbrushside_t *brushsides;
	size_t num_brushsides;
	mplane_t *planes;
	size_t num_planes;
	q2mapsurface_t *surfaces;
	size_t num_surfaces;
	size_t num_meshes;
	rottmaparea_t areas[NUM_AREATILES];
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

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
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

static qboolean is_window_index(int in)
{
	return in == 13;
}

static qboolean is_door_index(int in)
{
	if ((in >= 33) && (in <= 35))
		return true;
	else if ((in >= 90) && (in <= 104))
		return true;
	else if ((in >= 154) && (in <= 156))
		return true;
	else
		return false;
}

static qboolean is_maskwall_index(int in)
{
	if ((in >= 157) && (in <= 160))
		return true;
	else if ((in >= 162) && (in <= 179))
		return true;
	else
		return false;
}

// rt_door.h
#define MW_SHOOTABLE       0x01
#define MW_BLOCKING        0x02
#define MW_MULTI           0x04
#define MW_BLOCKINGCHANGES 0x08
#define MW_ABOVEPASSABLE   0x10
#define MW_NONDOGBLOCKING  0x20
#define MW_WEAPONBLOCKING  0x40
#define MW_BOTTOMPASSABLE  0x80
#define MW_MIDDLEPASSABLE  0x100
#define MW_ABP             0x200
#define MW_SWITCHON        0x400
#define MW_BOTTOMFLIPPING  0x800
#define MW_TOPFLIPPING     0x1000

static int get_maskwall_flags(int in)
{
	switch (in)
	{
		case 157: return MW_BLOCKING;
		case 158: return MW_MULTI|MW_BLOCKING|MW_BLOCKINGCHANGES|MW_SHOOTABLE;
		case 159: return MW_MULTI|MW_BLOCKING|MW_BLOCKINGCHANGES|MW_SHOOTABLE;
		case 160: return MW_MULTI|MW_BLOCKING|MW_BLOCKINGCHANGES|MW_SHOOTABLE;
		// 161 was removed
		case 162: return MW_SHOOTABLE|MW_BLOCKING;
		case 163: return MW_BLOCKING;
		case 164: return MW_SHOOTABLE|MW_BLOCKING;
		case 165: return MW_BLOCKING;
		case 166: return MW_SHOOTABLE|MW_BLOCKING;
		case 167: return MW_BLOCKING;
		case 168: return MW_SHOOTABLE|MW_BLOCKINGCHANGES|MW_BLOCKING;
		case 169: return MW_BOTTOMPASSABLE;
		case 170: return MW_NONDOGBLOCKING|MW_WEAPONBLOCKING;
		case 171: return MW_WEAPONBLOCKING|MW_BLOCKING;
		case 172: return MW_BOTTOMPASSABLE;
		case 173: return MW_BOTTOMPASSABLE;
		case 174: return MW_BLOCKING;
		case 175: return MW_BLOCKING|MW_SWITCHON;
		case 176: return MW_BOTTOMPASSABLE;
		case 177: return MW_BOTTOMPASSABLE;
		case 178: return MW_BOTTOMPASSABLE;
		case 179: return MW_ABOVEPASSABLE|MW_MIDDLEPASSABLE;
		default: return 0;
	}
}

static const char *get_maskwall_texture(int in)
{
	switch (in)
	{
		case 157: return "HSWITCH3";
		case 158: return "MULTI1";
		case 159: return "MULTI2";
		case 160: return "MULTI3";
		// 161 was removed
		case 162: return "MASKED1";
		case 163: return "MASKED1";
		case 164: return "MASKED2";
		case 165: return "MASKED2";
		case 166: return "MASKED3";
		case 167: return "MASKED3";
		case 168: return "MASKED4";
		case 169: return "MASKED4";
		case 170: return "DOGMASK";
		case 171: return "PEEPMASK";
		case 172: return "EXITARCH";
		case 173: return "EXITARCA";
		case 174: return "ENTRARCH";
		case 175: return "HSWITCH4";
		case 176: return "MULTI1";
		case 177: return "MULTI2";
		case 178: return "MULTI3";
		case 179: return "RAILING";
		default: return NULL;
	}
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

static int get_level_height(int in)
{
	if (in >= 90 && in <= 97)
		return in - 89;
	else if (in >= 450 && in <= 457)
		return in - 449;
	else
		return 1;
}

static const char *get_sky_texture(int in)
{
	if (!is_sky_index(in))
		return NULL;

	return rott_skies_texnames[in - 234];
}

static int get_level_brightness(int in)
{
	if (in >= 216 && in <= 223)
		return in - 216;
	return 7;
}

static qboolean tile_for_point(const vec3_t point, int *x, int *y)
{
	int tx, ty;
	tx = (point[0] + 4096) / 64;
	ty = (point[1] + 4096) / 64;
	if (x) *x = tx;
	if (y) *y = ty;
	return tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT;
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

static int ROTT_ClusterForPoint(model_t *model, const vec3_t point, int *areaout)
{
	if (areaout) *areaout = 0;
	return -1;
}

static qbyte *ROTT_ClusterPVS(model_t *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{
	return NULL;
}

static unsigned int ROTT_FatPVS(model_t *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{
	return 0;
}

static qboolean ROTT_EdictInFatPVS(model_t *model, const struct pvscache_s *edict, const qbyte *pvs, const int *areas)
{
	return true;
}

static void ROTT_FindTouchedLeafs(model_t *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs)
{

}

static void ROTT_LightPointValues(model_t *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	VectorSet(res_diffuse, 255, 255, 255);
	VectorSet(res_ambient, 128, 128, 128);
	VectorSet(res_dir, 0, 0, 1);
}

static entity_t pr_worldentity;

static void ROTT_PrepareFrame(model_t *mod, refdef_t *refdef, int area, int clusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out)
{
	*entvis_out = *surfvis_out = ROTT_ClusterPVS(mod, clusters[0], vis, false);
	if (clusters[1] != -1)
		ROTT_ClusterPVS(mod, clusters[1], vis, true);

	memset(&pr_worldentity, 0, sizeof(pr_worldentity));
	modfuncs->AngleVectors(pr_worldentity.angles, pr_worldentity.axis[0], pr_worldentity.axis[1], pr_worldentity.axis[2]);
	VectorInverse(pr_worldentity.axis[1]);
	pr_worldentity.model = mod;
	Vector4Set(pr_worldentity.shaderRGBAf, 1, 1, 1, 1);
	VectorSet(pr_worldentity.light_avg, 1, 1, 1);

	for (int i = 0; i < SHADER_SORT_COUNT; i++)
	for (batch_t *batch = mod->batches[i]; batch; batch = batch->next)
	{
		batch->firstmesh = 0;
		batch->meshes = batch->maxmeshes;
		batch->ent = &pr_worldentity;
	}
}

static void ROTT_GenerateVBOs(void *ctx, void *data, size_t a, size_t barg)
{
	model_t *mod = (model_t *)ctx;
	batch_t *b = mod->batches[0];
	int surf;
	mod->batches[0] = NULL;

	for (surf = 0; surf < mod->numbatches; surf++)
		mod->numsurfaces += b[surf].meshes;

	mod->texinfo = plugfuncs->GMalloc(&mod->memgroup, sizeof(*mod->texinfo) * mod->numsurfaces);
	mod->surfaces = plugfuncs->GMalloc(&mod->memgroup, sizeof(*mod->surfaces) * mod->numsurfaces);
	mod->firstmodelsurface = mod->nummodelsurfaces = 0;

	for (surf = 0; surf < mod->numbatches; surf++)
	{
		// FIXME: diffuse=_d normalmap=_local specular=_s
		b[surf].shader = modfuncs->RegisterBasicShader(mod, b[surf].texture->name, SUF_NONE, NULL, PTI_INVALID, 0, 0, NULL, NULL);
		// R_BuildDefaultTexnums_Doom3(b[surf].shader);

		// now we know its sort key, we can link it properly. *sigh*
		b[surf].next = mod->batches[b[surf].shader->sort];
		mod->batches[b[surf].shader->sort] = &b[surf];

		// all this extra stuff so r_showshaders works. *sigh*
		mod->surfaces[mod->nummodelsurfaces].texinfo = &mod->texinfo[mod->nummodelsurfaces];
		mod->surfaces[mod->nummodelsurfaces].texinfo->texture = b[surf].texture;
		mod->surfaces[mod->nummodelsurfaces].mesh = b[surf].mesh[0];
		mod->nummodelsurfaces++;
	}

	modfuncs->Batches_Build(mod, NULL);
}

static q2mapsurface_t *area_surface_for_name(rottmaparea_t *area, const char *texname)
{
	for (int i = 0; i < area->num_surfaces; i++)
		if (strncmp(area->surfaces[i].rname, texname, sizeof(area->surfaces[i].rname)) == 0)
			return &area->surfaces[i];
	return NULL;
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

static void add_cube_brush(model_t *mod, int i, int x, int y, int z, int width, int height, int depth, const char *texname, int contents)
{
	rottmapinfo_t *prv = (rottmapinfo_t *)mod->meshinfo;
	q2mapsurface_t *surface = surface_for_name(prv, texname);

	prv->brushes[i].contents = contents;
	VectorSet(prv->brushes[i].absmins, x * 64, y * 64, z * 64);
	VectorSet(prv->brushes[i].absmaxs, (x * 64) + (width * 64), (y * 64) + (height * 64), (z * 64) + (depth * 64));
	prv->brushes[i].numsides = 6;
	prv->brushes[i].brushside = &prv->brushsides[i * 6];

	// offset the whole map to center it in the grid space
	prv->brushes[i].absmins[0] -= 4096;
	prv->brushes[i].absmins[1] -= 4096;

	prv->brushes[i].absmaxs[0] -= 4096;
	prv->brushes[i].absmaxs[1] -= 4096;

	// add to bounds
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

// dir is 0=north 1=east 2=south 3=west 4=up 5=down
static void add_mesh_face(model_t *mod, mesh_t *mesh, int base, int x, int y, int dir, int neighbor_tile, int level_height)
{
	int i;
	index_t v[4];
	rottmapinfo_t *prv = (rottmapinfo_t *)mod->meshinfo;

	// vertices
	for (i = 0; i < 4; i++)
	{
		int ofs = base * 4 + i;

		switch (dir)
		{
			case 0: // north
				if (i == 0 || i == 3)
					mesh->xyz_array[ofs][0] = (x * 64);
				else
					mesh->xyz_array[ofs][0] = (x * 64) + 64;
				mesh->xyz_array[ofs][1] = (y * 64) + 64;
				if (is_maskwall_index(neighbor_tile))
					mesh->xyz_array[ofs][1] += 32;
				break;

			case 1: // east
				mesh->xyz_array[ofs][0] = (x * 64) + 64;
				if (i == 0 || i == 3)
					mesh->xyz_array[ofs][1] = (y * 64);
				else
					mesh->xyz_array[ofs][1] = (y * 64) + 64;
				if (is_maskwall_index(neighbor_tile))
					mesh->xyz_array[ofs][0] += 32;
				break;

			case 2: // south
				if (i == 0 || i == 3)
					mesh->xyz_array[ofs][0] = (x * 64);
				else
					mesh->xyz_array[ofs][0] = (x * 64) + 64;
				mesh->xyz_array[ofs][1] = (y * 64);
				if (is_maskwall_index(neighbor_tile))
					mesh->xyz_array[ofs][1] -= 32;
				break;

			case 3: // west
				mesh->xyz_array[ofs][0] = (x * 64);
				if (i == 0 || i == 3)
					mesh->xyz_array[ofs][1] = (y * 64);
				else
					mesh->xyz_array[ofs][1] = (y * 64) + 64;
				if (is_maskwall_index(neighbor_tile))
					mesh->xyz_array[ofs][0] -= 32;
				break;

			case 4: // up
			case 5: // down
				if (i == 0 || i == 3)
					mesh->xyz_array[ofs][0] = (x * 64);
				else
					mesh->xyz_array[ofs][0] = (x * 64) + 64;
				if (i == 0 || i == 1)
					mesh->xyz_array[ofs][1] = (y * 64);
				else
					mesh->xyz_array[ofs][1] = (y * 64) + 64;
				break;
		}

		if (dir == 4 || dir == 5)
		{
			if (dir == 4)
				mesh->xyz_array[ofs][2] = level_height * 64;
			else if (dir == 5)
				mesh->xyz_array[ofs][2] = 0;

			if (i == 0 || i == 3)
				mesh->st_array[ofs][0] = 0;
			else
				mesh->st_array[ofs][0] = 1;
			if (i >= 2)
				mesh->st_array[ofs][1] = 1;
			else
				mesh->st_array[ofs][1] = 0;
		}
		else
		{
			if (i >= 2)
				mesh->xyz_array[ofs][2] = 0;
			else
				mesh->xyz_array[ofs][2] = level_height * 64;

			// setup texcoords
			if (i == 0 || i == 3)
				mesh->st_array[ofs][0] = 0;
			else
				mesh->st_array[ofs][0] = 1;
			if (i >= 2)
				mesh->st_array[ofs][1] = level_height;
			else
				mesh->st_array[ofs][1] = 0;

			// handle maskwall flags
			if (is_maskwall_index(neighbor_tile))
			{
				int flags = get_maskwall_flags(neighbor_tile);

				if (flags & MW_ABOVEPASSABLE || flags & MW_MIDDLEPASSABLE)
				{
					if (i >= 2)
						mesh->st_array[ofs][1] = 1;
					else
						mesh->xyz_array[ofs][2] = 64;
				}
			}
		}

		// offset the whole map to center it in the grid space
		mesh->xyz_array[ofs][0] -= 4096;
		mesh->xyz_array[ofs][1] -= 4096;

		v[i] = ofs;
	}

	// faces
	if (dir == 0 || dir == 3)
	{
		mesh->indexes[base * 6 + 0] = v[1];
		mesh->indexes[base * 6 + 1] = v[2];
		mesh->indexes[base * 6 + 2] = v[3];

		mesh->indexes[base * 6 + 3] = v[0];
		mesh->indexes[base * 6 + 4] = v[1];
		mesh->indexes[base * 6 + 5] = v[3];
	}
	else if (dir == 4)
	{
		mesh->indexes[base * 6 + 0] = v[0];
		mesh->indexes[base * 6 + 1] = v[1];
		mesh->indexes[base * 6 + 2] = v[2];

		mesh->indexes[base * 6 + 3] = v[0];
		mesh->indexes[base * 6 + 4] = v[2];
		mesh->indexes[base * 6 + 5] = v[3];
	}
	else
	{
		mesh->indexes[base * 6 + 0] = v[3];
		mesh->indexes[base * 6 + 1] = v[2];
		mesh->indexes[base * 6 + 2] = v[1];

		mesh->indexes[base * 6 + 3] = v[3];
		mesh->indexes[base * 6 + 4] = v[1];
		mesh->indexes[base * 6 + 5] = v[0];
	}
}

static qboolean QDECL Mod_LoadROTTModel(model_t *mod, void *buffer, size_t fsize)
{
	int i, j, k;
	int x, y, z;
	int len_entities;
	char *entities;
	char *entptr;
	struct bihleaf_s *bihleaf;
	mrottmap_t *header;
	rottmapinfo_t *prv;
	q2mapsurface_t *areasurfaces[NUM_AREATILES];
	q2mapsurface_t *surfaces;
	batch_t *b;
	int level_floor, level_ceiling, level_height;
	const char *level_floor_texname, *level_ceiling_texname;
	int *meshoffsets;
	int *areameshoffsets[NUM_AREATILES];
	int *num_meshes_per_surface;

	// sanity check
	if (memcmp(buffer, "ROTT", 4) != 0)
	{
		Con_Printf(CON_ERROR"%s: format not recognised\n", mod->name);
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

	// get floor and ceiling textures
	level_floor = prv->mapplanes[0][0][0];
	level_ceiling = prv->mapplanes[0][0][1];
	level_floor_texname = convert_updn_texture(level_floor);
	level_ceiling_texname = is_sky_index(level_ceiling) ? get_sky_texture(level_ceiling) : convert_updn_texture(level_ceiling);

	// create surfaces in each area
	memset(areasurfaces, 0, sizeof(areasurfaces));
	memset(prv->areas, 0, sizeof(prv->areas));
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			int neighbors[4]; // n e s w
			int infoneighbors[4]; // n e s w
			int area = prv->mapplanes[0][y][x] - FIRST_AREATILE;
			if (area < 0 || area >= NUM_AREATILES)
				continue;

			neighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[0][y + 1][x] : -1;
			neighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[0][y][x + 1] : -1;
			neighbors[2] = y > 0 ? prv->mapplanes[0][y - 1][x] : -1;
			neighbors[3] = x > 0 ? prv->mapplanes[0][y][x - 1] : -1;

			infoneighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[2][y + 1][x] : -1;
			infoneighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[2][y][x + 1] : -1;
			infoneighbors[2] = y > 0 ? prv->mapplanes[2][y - 1][x] : -1;
			infoneighbors[3] = x > 0 ? prv->mapplanes[2][y][x - 1] : -1;

			// create neighboring wall surfaces
			for (i = 0; i < 4; i++)
			{
				const char *texname = get_wall_texture(neighbors[i]);
				if (is_maskwall_index(neighbors[i]))
					texname = get_maskwall_texture(neighbors[i]);
				if (is_window_index(infoneighbors[i]))
					texname = level_ceiling_texname;
				if (!texname)
					continue;
				add_surface(&areasurfaces[area], &prv->areas[area].num_surfaces, texname);
			}

			// create floor and ceiling surfaces
			add_surface(&areasurfaces[area], &prv->areas[area].num_surfaces, level_floor_texname);
			add_surface(&areasurfaces[area], &prv->areas[area].num_surfaces, level_ceiling_texname);
		}
	}

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
			if (is_maskwall_index(prv->mapplanes[0][y][x]))
				texname = get_maskwall_texture(prv->mapplanes[0][y][x]);
			if (is_window_index(prv->mapplanes[2][y][x]))
				texname = level_ceiling_texname;
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
	level_height = get_level_height(prv->mapplanes[1][0][0]);

	// add a few more surfaces
	add_surface(&surfaces, &prv->num_surfaces, level_floor_texname);
	add_surface(&surfaces, &prv->num_surfaces, level_ceiling_texname);

	// copy out area surfaces
	for (i = 0; i < NUM_AREATILES; i++)
	{
		if (areasurfaces[i] && prv->areas[i].num_surfaces)
		{
			prv->areas[i].surfaces = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->areas[i].surfaces) * prv->areas[i].num_surfaces);
			memcpy(prv->areas[i].surfaces, areasurfaces[i], sizeof(*prv->areas[i].surfaces) * prv->areas[i].num_surfaces);
			plugfuncs->Free(areasurfaces[i]);
		}
	}

	// copy out surfaces
	prv->surfaces = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->surfaces) * prv->num_surfaces);
	memcpy(prv->surfaces, surfaces, sizeof(*prv->surfaces) * prv->num_surfaces);
	plugfuncs->Free(surfaces);

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// setup floor and ceiling brushes
	add_cube_brush(mod, 0, 0, 0, -1, MAP_WIDTH, MAP_HEIGHT, 1, level_floor_texname, FTECONTENTS_SOLID);
	add_cube_brush(mod, 1, 0, 0, level_height, MAP_WIDTH, MAP_HEIGHT, 1, level_ceiling_texname, is_sky_index(level_ceiling) ? FTECONTENTS_SKY : FTECONTENTS_SOLID);

	// setup other brushes
	// start at 2 to skip floor and ceiling brushes
	i = 2;
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			const char *texname = get_wall_texture(prv->mapplanes[0][y][x]);
			if (is_maskwall_index(prv->mapplanes[0][y][x]))
				texname = get_maskwall_texture(prv->mapplanes[0][y][x]);
			if (is_window_index(prv->mapplanes[2][y][x]))
				texname = level_ceiling_texname;
			if (!texname)
				continue;
			add_cube_brush(mod, i, x, y, 0, 1, 1, level_height, texname, is_window_index(prv->mapplanes[2][y][x]) ? FTECONTENTS_SKY : FTECONTENTS_SOLID);
			i++;
		}
	}

	// process each area, creating meshes within each one
	prv->num_meshes = 0;
	for (i = 0; i < NUM_AREATILES; i++)
	{
		if (!prv->areas[i].num_surfaces || !prv->areas[i].surfaces)
			continue;

		prv->areas[i].num_meshes = prv->areas[i].num_surfaces;
		prv->areas[i].meshes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->areas[i].meshes) * prv->areas[i].num_meshes);
		prv->num_meshes += prv->areas[i].num_meshes;
	}

	// allocate batches
	b = plugfuncs->GMalloc(&mod->memgroup, sizeof(*b) * prv->num_surfaces);
	mod->numsurfaces = prv->num_surfaces;

	// accumulate vertices and indexes for each area
	// FIXME: doesn't prune hidden or parallel faces
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			q2mapsurface_t *surface;
			mesh_t *mesh;
			int neighbors[4]; // n e s w
			int infoneighbors[4]; // n e s w
			int area = prv->mapplanes[0][y][x] - FIRST_AREATILE;
			if (is_window_index(prv->mapplanes[2][y][x]))
				continue;
			if (area < 0 || area >= NUM_AREATILES)
				continue;
			if (!prv->areas[area].num_surfaces || !prv->areas[area].surfaces)
				continue;

			neighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[0][y + 1][x] : -1;
			neighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[0][y][x + 1] : -1;
			neighbors[2] = y > 0 ? prv->mapplanes[0][y - 1][x] : -1;
			neighbors[3] = x > 0 ? prv->mapplanes[0][y][x - 1] : -1;

			infoneighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[2][y + 1][x] : -1;
			infoneighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[2][y][x + 1] : -1;
			infoneighbors[2] = y > 0 ? prv->mapplanes[2][y - 1][x] : -1;
			infoneighbors[3] = x > 0 ? prv->mapplanes[2][y][x - 1] : -1;

			// neighboring walls
			for (i = 0; i < 4; i++)
			{
				const char *texname = get_wall_texture(neighbors[i]);
				if (is_maskwall_index(neighbors[i]))
					texname = get_maskwall_texture(neighbors[i]);
				if (is_window_index(infoneighbors[i]))
					texname = level_ceiling_texname;
				if (!texname)
					continue;
				surface = area_surface_for_name(&prv->areas[area], texname);
				mesh = &prv->areas[area].meshes[surface - prv->areas[area].surfaces];
				mesh->numvertexes += 4;
				mesh->numindexes += 6;
			}

			// floor
			surface = area_surface_for_name(&prv->areas[area], level_floor_texname);
			mesh = &prv->areas[area].meshes[surface - prv->areas[area].surfaces];
			mesh->numvertexes += 4;
			mesh->numindexes += 6;

			// ceiling
			surface = area_surface_for_name(&prv->areas[area], level_ceiling_texname);
			mesh = &prv->areas[area].meshes[surface - prv->areas[area].surfaces];
			mesh->numvertexes += 4;
			mesh->numindexes += 6;
		}
	}

	// allocate mesh arrays for each area
	// we use this array to track how many meshes each batch has
	num_meshes_per_surface = plugfuncs->Malloc(sizeof(*num_meshes_per_surface) * prv->num_surfaces);
	memset(num_meshes_per_surface, 0, sizeof(*num_meshes_per_surface) * prv->num_surfaces);
	for (i = 0; i < NUM_AREATILES; i++)
	{
		for (j = 0; j < prv->areas[i].num_meshes; j++)
		{
			int s;
			uint8_t *vdata = plugfuncs->GMalloc(&mod->memgroup, prv->areas[i].meshes[j].numvertexes * (sizeof(vecV_t) + sizeof(vec2_t) + sizeof(vec3_t) * 3 + sizeof(vec4_t)) + prv->areas[i].meshes[j].numindexes * sizeof(index_t));

			prv->areas[i].meshes[j].colors4f_array[0] = (vec4_t *)vdata; vdata += sizeof(vec4_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].xyz_array = (vecV_t *)vdata; vdata += sizeof(vecV_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].st_array = (vec2_t *)vdata; vdata += sizeof(vec2_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].normals_array = (vec3_t *)vdata; vdata += sizeof(vec3_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].snormals_array = (vec3_t *)vdata; vdata += sizeof(vec3_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].tnormals_array = (vec3_t *)vdata; vdata += sizeof(vec3_t) * prv->areas[i].meshes[j].numvertexes;
			prv->areas[i].meshes[j].indexes = (index_t *)vdata;

			s = surface_for_name(prv, prv->areas[i].surfaces[j].rname) - prv->surfaces;

			num_meshes_per_surface[s]++;
		}
	}

	// setup batches
	for (i = 0; i < prv->num_surfaces; i++)
	{
		b[i].maxmeshes = num_meshes_per_surface[i];
		b[i].mesh = plugfuncs->GMalloc(&mod->memgroup, sizeof(*b[i].mesh) * b[i].maxmeshes);
		b[i].meshes = 0;
		b[i].firstmesh = 0;
		b[i].lightmap[0] = -1;
		b[i].lightmap[1] = -1;
		b[i].lightmap[2] = -1;
		b[i].lightmap[3] = -1;
		b[i].lmlightstyle[0] = 0;
		b[i].lmlightstyle[1] = INVALID_LIGHTSTYLE;
		b[i].lmlightstyle[2] = INVALID_LIGHTSTYLE;
		b[i].lmlightstyle[3] = INVALID_LIGHTSTYLE;

		b[i].texture = plugfuncs->GMalloc(&mod->memgroup, sizeof(*b[i].texture));
		Q_strlcpy(b[i].texture->name, prv->surfaces[i].rname, sizeof(b[i].texture->name));
	}

	plugfuncs->Free(num_meshes_per_surface);

	// we use this array to track how many faces have been added to each mesh
	meshoffsets = plugfuncs->Malloc(sizeof(*meshoffsets) * prv->num_meshes);
	memset(areameshoffsets, 0, sizeof(areameshoffsets));
	memset(meshoffsets, 0, sizeof(*meshoffsets) * prv->num_meshes);
	j = 0;
	for (i = 0; i < NUM_AREATILES; i++)
	{
		if (!prv->areas[i].num_meshes)
			continue;
		areameshoffsets[i] = &meshoffsets[j];
		j += prv->areas[i].num_meshes;
	}

	// read in mesh data
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			index_t v[4];
			mesh_t *mesh;
			int s;
			int neighbors[4]; // n e s w
			int infoneighbors[4]; // n e s w
			int area = prv->mapplanes[0][y][x] - FIRST_AREATILE;
			if (is_window_index(prv->mapplanes[2][y][x]))
				continue;
			if (area < 0 || area >= NUM_AREATILES)
				continue;
			if (!prv->areas[area].num_meshes || !prv->areas[area].meshes)
				continue;

			neighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[0][y + 1][x] : -1;
			neighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[0][y][x + 1] : -1;
			neighbors[2] = y > 0 ? prv->mapplanes[0][y - 1][x] : -1;
			neighbors[3] = x > 0 ? prv->mapplanes[0][y][x - 1] : -1;

			infoneighbors[0] = y < MAP_HEIGHT - 1 ? prv->mapplanes[2][y + 1][x] : -1;
			infoneighbors[1] = x < MAP_WIDTH - 1 ? prv->mapplanes[2][y][x + 1] : -1;
			infoneighbors[2] = y > 0 ? prv->mapplanes[2][y - 1][x] : -1;
			infoneighbors[3] = x > 0 ? prv->mapplanes[2][y][x - 1] : -1;

			// neighboring walls
			for (i = 0; i < 4; i++)
			{
				const char *texname = get_wall_texture(neighbors[i]);
				if (is_maskwall_index(neighbors[i]))
					texname = get_maskwall_texture(neighbors[i]);
				if (is_window_index(infoneighbors[i]))
					texname = level_ceiling_texname;
				if (!texname)
					continue;
				s = area_surface_for_name(&prv->areas[area], texname) - prv->areas[area].surfaces;
				mesh = &prv->areas[area].meshes[s];

				add_mesh_face(mod, mesh, areameshoffsets[area][s], x, y, i, neighbors[i], level_height);

				areameshoffsets[area][s]++;
			}

			// floor
			s = area_surface_for_name(&prv->areas[area], level_floor_texname) - prv->areas[area].surfaces;
			mesh = &prv->areas[area].meshes[s];

			add_mesh_face(mod, mesh, areameshoffsets[area][s], x, y, 5, 0, level_height);

			areameshoffsets[area][s]++;

			// ceiling
			s = area_surface_for_name(&prv->areas[area], level_ceiling_texname) - prv->areas[area].surfaces;
			mesh = &prv->areas[area].meshes[s];

			add_mesh_face(mod, mesh, areameshoffsets[area][s], x, y, 4, 0, level_height);

			areameshoffsets[area][s]++;
		}
	}
	plugfuncs->Free(meshoffsets);

	// populate batch mesh lists
	for (i = 0; i < NUM_AREATILES; i++)
	{
		for (j = 0; j < prv->areas[i].num_meshes; j++)
		{
			int s = surface_for_name(prv, prv->areas[i].surfaces[j].rname) - prv->surfaces;
			b[s].mesh[b[s].meshes++] = &prv->areas[i].meshes[j];
		}
	}

	// build batches
	mod->fromgame = fg_new;
	mod->type = mod_brush;
	mod->lightmaps.surfstyles = 1;
	memset(mod->batches, 0, sizeof(mod->batches));
	mod->batches[0] = b;
	mod->numbatches = prv->num_surfaces;
	threadfuncs->AddWork(WG_MAIN, ROTT_GenerateVBOs, mod, NULL, MLS_LOADED, 0);

	// build BIH tree
	bihleaf = plugfuncs->Malloc(sizeof(*bihleaf) * prv->num_brushes);
	for (i = 0; i < prv->num_brushes; i++)
	{
		bihleaf[i].type = BIH_BRUSH;
		bihleaf[i].data.contents = prv->brushes[i].contents;
		bihleaf[i].data.brush = &prv->brushes[i];
	}
	modfuncs->BIH_Build(mod, bihleaf, prv->num_brushes);
	plugfuncs->Free(bihleaf);

	// add entities
	len_entities = 8192;
	entptr = entities = plugfuncs->Malloc(len_entities);
	Q_snprintfz(entities, len_entities, "{\n\"classname\"\"worldspawn\"\n}\n{\n\"classname\"\"info_player_start\"\"origin\"\"0 0 512\"\n}\n");
	modfuncs->LoadEntities(mod, entities, len_entities);
	plugfuncs->Free(entities);

	// set funcs
	mod->funcs.FatPVS = ROTT_FatPVS;
	mod->funcs.EdictInFatPVS = ROTT_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = ROTT_FindTouchedLeafs;
	mod->funcs.LightPointValues = ROTT_LightPointValues;
	mod->funcs.ClusterForPoint = ROTT_ClusterForPoint;
	mod->funcs.ClusterPVS = ROTT_ClusterPVS;
	mod->funcs.PrepareFrame = ROTT_PrepareFrame;

	// finish up
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	mod->flags = 0;

	return true;
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
