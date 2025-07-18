/*
 * written by erysdren (it/its) in 2025
 *
 * https://wiki.beyondunreal.com/Unreal_package
 * https://www.unrealarchive.org/wikis/unreal-wiki/Legacy:Package_File_Format.html
 */

#include "../plugin.h"
#include "../../engine/common/fs.h"

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

typedef struct rottmapset {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	char filename[MAX_OSPATH];
	vfsfile_t *handle;
	drottmap_t maps[NUM_MAPS];
	fsbucket_t buckets[NUM_MAPS];
	void *mutex;
	int references;
} rottmapset_t;

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

static void QDECL FSUPKG_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSUPKG_ClosePath(searchpathfuncs_t *handle)
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

static void QDECL FSUPKG_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;
	for (i = 0; i < NUM_MAPS; i++)
	{
		char shortname[16];
		char mapname[MAX_OSPATH];
		if (!pak->maps[i].used)
			continue;
		Q_snprintfz(mapname, sizeof(mapname), "%s.%s.map", pak->filename, level_shortname_for_index(shortname, sizeof(shortname), i));
		AddFileHash(depth, mapname, &pak->buckets[i], &pak->maps[i]);
	}
}

static unsigned int QDECL FSUPKG_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	drottmap_t *file = (drottmap_t *)hashedresult;
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
			char shortname[16];
			char mapname[MAX_OSPATH];
			if (!pak->maps[i].used)
				continue;
			Q_snprintfz(mapname, sizeof(mapname), "%s.%s.map", pak->filename, level_shortname_for_index(shortname, sizeof(shortname), i));
			if (strcasecmp(filename, mapname) == 0)
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
			loc->len = sizeof(mrottmap_t) + file->len_planes[0] + file->len_planes[1] + file->len_planes[2];
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

static int QDECL VFSUPKG_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;

	if (bytestoread + vfsp->pos > vfsp->len)
		bytestoread = vfsp->len - vfsp->pos;

	memcpy(buffer, (uint8_t *)vfsp->map + vfsp->pos, bytestoread);

	vfsp->pos += bytestoread;

	return bytestoread;
}

static int QDECL VFSUPKG_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to ROTT mapset files\n");
	return 0;
}

static qofs_t QDECL VFSUPKG_Tell(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->pos;
}

static qofs_t QDECL VFSUPKG_GetLen(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->len;
}

static qboolean QDECL VFSUPKG_Close(vfsfile_t *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	FSUPKG_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp->map); //free ourselves.
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSUPKG_Seek(struct vfsfile_s *vfs, qofs_t pos)
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
		len += pak->maps[idx].len_planes[i];

	map = plugfuncs->Malloc(len);

	// copy in map data
	memcpy(map, &pak->maps[idx], sizeof(pak->maps[idx]));

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
		VFS_SEEK(pak->handle, pak->maps[idx].ofs_planes[i]);
		VFS_READ(pak->handle, (uint8_t *)map + map->ofs_planes[i], map->len_planes[i]);
	}

	return map;
}

static vfsfile_t *QDECL FSUPKG_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	drottmap_t *map = (drottmap_t *)loc->fhandle;
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
	Q_strlcpy(vfs->funcs.dbgname, map->name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSUPKG_Close;
	vfs->funcs.GetLen = VFSUPKG_GetLen;
	vfs->funcs.ReadBytes = VFSUPKG_ReadBytes;
	vfs->funcs.Seek = VFSUPKG_Seek;
	vfs->funcs.Tell = VFSUPKG_Tell;
	vfs->funcs.WriteBytes = VFSUPKG_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSUPKG_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSUPKG_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSUPKG_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;

	for (i = 0; i < NUM_MAPS; i++)
	{
		char shortname[16];
		char mapname[MAX_OSPATH];
		if (!pak->maps[i].used)
			continue;
		Q_snprintfz(mapname, sizeof(mapname), "%s.%s.map", pak->filename, level_shortname_for_index(shortname, sizeof(shortname), i));
		if (filefuncs->WildCmp(match, mapname))
		{
			if (!func(mapname, sizeof(mrottmap_t) + pak->maps[i].len_planes[0] + pak->maps[i].len_planes[1] + pak->maps[i].len_planes[2], 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSUPKG_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	return filefuncs->BlockChecksum(pak->maps, sizeof(pak->maps));
}

static searchpathfuncs_t *QDECL FSUPKG_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	uint32_t magic, version;
	rottmapset_t *pak;
	int i, j;
	qboolean valid = false;

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
	if (VFS_READ(file, pak->maps, sizeof(pak->maps)) < sizeof(pak->maps))
		return NULL;

	// setup info
	pak->references++;
	pak->handle = file;
	Q_strlcpy(pak->filename, filename, sizeof(pak->filename));
	Q_strlcpy(pak->desc, desc, sizeof(pak->desc));
	VFS_SEEK(pak->handle, 0);
	pak->mutex = Sys_CreateMutex();

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSUPKG_GetPathDetails;
	pak->pub.ClosePath = FSUPKG_ClosePath;
	pak->pub.BuildHash = FSUPKG_BuildHash;
	pak->pub.FindFile = FSUPKG_FindFile;
	pak->pub.ReadFile = FSUPKG_ReadFile;
	pak->pub.EnumerateFiles = FSUPKG_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSUPKG_GeneratePureCRC;
	pak->pub.OpenVFS = FSUPKG_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

// might as well store them all here, there's a LOT of different extensions
// and since fs plugins can't operate by magic value, it's necessary
// FIXME: could be controlled by a cvar, like r_imageextensions?
static const char *upkg_extensions[] = {
	/* compiled unrealscript */ "u",
	/* maps                  */ "unr", "ut2", "dnf", "fuk",
	/* textures              */ "utx", "dtx",
	/* music                 */ "umx",
	/* sound                 */ "uax", "dfx",
	/* animations            */ "ukx",
	/* static meshes         */ "usx",
	/* unreal zip            */ "uz", "uz2",
	/* not sure              */ "dmx"
};

qboolean FSUPKG_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	for (int i = 0; i < countof(upkg_extensions); i++)
	{
		char name[128];
		Q_snprintfz(name, sizeof(name), "FS_RegisterArchiveType_%s", upkg_extensions[i]);
		if (!plugfuncs->ExportFunction(name, FSUPKG_LoadArchive)) return false;
	}

	return true;
}
