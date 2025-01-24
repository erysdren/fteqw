#include "../plugin.h"
#include "../../engine/common/fs.h"

#include "rott.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#undef Sys_CreateMutex
#undef Sys_LockMutex
#undef Sys_UnlockMutex
#undef Sys_DestroyMutex
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

static const char *rtl_magics[] = {
	"RTL\0", "RTC\0", "RTR\0",
	"RXL\0", "RXC\0"
};

typedef struct fsrottmap {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	drottmap_t map;
} fsrottmap_t;

typedef struct rottmapset {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	char filename[MAX_OSPATH];
	vfsfile_t *handle;
	fsrottmap_t maps[NUM_MAPS];
	void *mutex;
	int references;
} rottmapset_t;

#if 0
static int level_index_for_shortname(const char *shortname)
{
	int episode, level;

	if (sscanf(shortname, "E%dA%d", &episode, &level) != 2)
	{
		Con_Printf("rott: sscanf failed in level_index_for_shortname()\n");
		return -1;
	}

	// error checking
	if (episode < 1 || episode > 4)
	{
		Con_Printf("rott: episode %d is out of range\n", episode);
		return -1;
	}
	if (level < 1 || level > 99)
	{
		Con_Printf("rott: level %d is out of range\n", level);
		return -1;
	}

	if (episode == 4)
		return level + 23;
	else if (episode == 3)
		return level + 15;
	else if (episode == 2)
		return level + 7;
	else
		return level - 1;
}
#endif

static char *level_shortname_for_index(char *out, size_t outlen, int index)
{
	int episode, level;

	if (index < 0 || index >= 100)
	{
		Con_Printf("rott: index %d is out of range\n", index);
		return NULL;
	}

	if (index > 23)
	{
		episode = 4;
		level = index - 23;
	}
	else if (index > 15)
	{
		episode = 3;
		level = index - 15;
	}
	else if (index > 7)
	{
		episode = 2;
		level = index - 7;
	}
	else
	{
		episode = 1;
		level = index + 1;
	}

	Q_snprintfz(out, outlen, "E%dA%d", episode, level);

	return out;
}

static void QDECL FSROTT_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSROTT_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	rottmapset_t *pak = (rottmapset_t *)handle;

	if (!Sys_LockMutex(pak->mutex))
		return; //ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return; //not free yet

	VFS_CLOSE(pak->handle);

	Sys_DestroyMutex(pak->mutex);

	plugfuncs->Free(pak);
}

static void QDECL FSROTT_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;
	for (i = 0; i < NUM_MAPS; i++)
	{
		if (!pak->maps[i].map.used)
			continue;
		AddFileHash(depth, pak->maps[i].name, &pak->maps[i].bucket, &pak->maps[i]);
	}
}

static unsigned int QDECL FSROTT_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	fsrottmap_t *file = (fsrottmap_t *)hashedresult;
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;

	if (file)
	{
		//is this a pointer to a file in this pak?
		if (file < pak->maps || file > pak->maps + NUM_MAPS)
			return FF_NOTFOUND;
	}
	else
	{
		for (i = 0; i < NUM_MAPS; i++)
		{
			if (strcasecmp(filename, pak->maps[i].name) == 0)
			{
				file = &pak->maps[i];
				break;
			}
		}
	}

	if (file)
	{
		if (loc)
		{
			loc->fhandle = file;
			*loc->rawname = 0;
			loc->offset = (qofs_t)-1;
			loc->len = sizeof(mrottmap_t) + file->map.len_planes[0] + file->map.len_planes[1] + file->map.len_planes[2];
		}

		return FF_FOUND;
	}

	return FF_NOTFOUND;
}

typedef struct vfsrottmapset {
	vfsfile_t funcs;
	rottmapset_t *parent;
	mrottmap_t *map;
	qofs_t len;
	qofs_t pos;
} vfsrottmapset_t;

static int QDECL VFSROTT_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;

	if (bytestoread + vfsp->pos > vfsp->len)
		bytestoread = vfsp->len - vfsp->pos;

	memcpy(buffer, (uint8_t *)vfsp->map + vfsp->pos, bytestoread);

	vfsp->pos += bytestoread;

	return bytestoread;
}

static int QDECL VFSROTT_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to ROTT mapset files\n");
	return 0;
}

static qofs_t QDECL VFSROTT_Tell(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->pos;
}

static qofs_t QDECL VFSROTT_GetLen(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->len;
}

static qboolean QDECL VFSROTT_Close(vfsfile_t *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	FSROTT_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp->map); //free ourselves.
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSROTT_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	if (pos > vfsp->len)
		return false;
	vfsp->pos = pos;
	return true;
}

static mrottmap_t *construct_standalone_rott_map(rottmapset_t *pak, int idx)
{
	mrottmap_t *map = NULL;
	size_t len = sizeof(mrottmap_t);

	for (int i = 0; i < 3; i++)
		len += pak->maps[idx].map.len_planes[i];

	map = plugfuncs->Malloc(len);

	// copy in map data
	memcpy(map, &pak->maps[idx].map, sizeof(pak->maps[idx]));

	// setup magic
	map->magic[0] = 'R';
	map->magic[1] = 'O';
	map->magic[2] = 'T';
	map->magic[3] = 'T';

	// adjust offsets
	map->ofs_planes[0] = sizeof(mrottmap_t);
	map->ofs_planes[1] = sizeof(mrottmap_t) + map->len_planes[0];
	map->ofs_planes[2] = sizeof(mrottmap_t) + map->len_planes[0] + map->len_planes[1];

	// copy plane data
	for (int i = 0; i < 3; i++)
	{
		VFS_SEEK(pak->handle, pak->maps[idx].map.ofs_planes[i]);
		VFS_READ(pak->handle, (uint8_t *)map + map->ofs_planes[i], map->len_planes[i]);
	}

	return map;
}

static vfsfile_t *QDECL FSROTT_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	fsrottmap_t *map = (fsrottmap_t *)loc->fhandle;
	vfsrottmapset_t *vfs;
	int mapidx = 0;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	// get map index
	mapidx = map - pak->maps;
	if (mapidx < 0 || mapidx >= 100)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsrottmapset_t));

	vfs->parent = pak;
	if (!Sys_LockMutex(vfs->parent->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}

	// construct memory map
	vfs->map = construct_standalone_rott_map(pak, mapidx);
	vfs->len = loc->len;
	vfs->pos = 0;

	vfs->parent->references++;
	Sys_UnlockMutex(vfs->parent->mutex);

#ifdef _DEBUG
	Q_strlcpy(vfs->funcs.dbgname, map->map.name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSROTT_Close;
	vfs->funcs.GetLen = VFSROTT_GetLen;
	vfs->funcs.ReadBytes = VFSROTT_ReadBytes;
	vfs->funcs.Seek = VFSROTT_Seek;
	vfs->funcs.Tell = VFSROTT_Tell;
	vfs->funcs.WriteBytes = VFSROTT_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSROTT_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSROTT_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSROTT_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;

	for (i = 0; i < NUM_MAPS; i++)
	{
		if (!pak->maps[i].map.used)
			continue;
		if (filefuncs->WildCmp(match, pak->maps[i].name))
			if (!func(pak->maps[i].name, sizeof(mrottmap_t) + pak->maps[i].map.len_planes[0] + pak->maps[i].map.len_planes[1] + pak->maps[i].map.len_planes[2], 0, parm, handle))
				return false;
	}

	return true;
}

static int QDECL FSROTT_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	return filefuncs->BlockChecksum(pak->maps, sizeof(pak->maps));
}

static searchpathfuncs_t *QDECL FSROTT_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	uint32_t magic, version;
	rottmapset_t *pak;
	int i;
	qboolean valid = false;
	char shortname[16];

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// validate magic
	if (VFS_READ(file, &magic, sizeof(magic)) < sizeof(magic))
	{
		Con_Printf("rott: failed read\n");
		return NULL;
	}
	for (i = 0; i < countof(rtl_magics); i++)
		if (memcmp(rtl_magics[i], &magic, 4) == 0)
			valid = true;
	if (!valid)
	{
		Con_Printf("rott: unrecognized magic 0x%08x\n", magic);
		return NULL;
	}

	// validate version
	if (VFS_READ(file, &version, sizeof(version)) < sizeof(version))
	{
		Con_Printf("rott: failed read\n");
		return NULL;
	}
	version = LittleLong(version);
	if (version != rtl_version && version != rxl_version)
	{
		Con_Printf("rott: unrecognized version %d\n", version);
		return NULL;
	}

	// alloc
	pak = (rottmapset_t *)plugfuncs->Malloc(sizeof(*pak));

	// read maps
	for (i = 0; i < NUM_MAPS; i++)
		if (VFS_READ(file, &pak->maps[i].map, sizeof(drottmap_t)) < sizeof(drottmap_t))
			return NULL;

	// setup info
	pak->references++;
	pak->handle = file;
	Q_strlcpy(pak->filename, filename, sizeof(pak->filename));
	Q_strlcpy(pak->desc, desc, sizeof(pak->desc));
	VFS_SEEK(pak->handle, 0);
	pak->mutex = Sys_CreateMutex();

	// setup names
	for (i = 0; i < NUM_MAPS; i++)
		Q_snprintfz(pak->maps[i].name, sizeof(pak->maps[i].name), "maps/%s.%s.map", pak->filename, level_shortname_for_index(shortname, sizeof(shortname), i));

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSROTT_GetPathDetails;
	pak->pub.ClosePath = FSROTT_ClosePath;
	pak->pub.BuildHash = FSROTT_BuildHash;
	pak->pub.FindFile = FSROTT_FindFile;
	pak->pub.ReadFile = FSROTT_ReadFile;
	pak->pub.EnumerateFiles = FSROTT_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSROTT_GeneratePureCRC;
	pak->pub.OpenVFS = FSROTT_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

qboolean FSROTT_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_rtl", FSROTT_LoadArchive)) return false;
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_rtc", FSROTT_LoadArchive)) return false;
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_rtr", FSROTT_LoadArchive)) return false;
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_rtlx", FSROTT_LoadArchive)) return false;
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_rtcx", FSROTT_LoadArchive)) return false;

	return true;
}
