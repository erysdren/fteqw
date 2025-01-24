#include "../plugin.h"
#include "../../engine/common/fs.h"

#ifndef ASIZE
#define ASIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

static const uint32_t rtl_version = 0x0101;
static const uint32_t rxl_version = 0x0200;
static const uint32_t rle_tag_registered = 0x4344;
static const uint32_t rle_tag_shareware = 0x4d4b;

static const char *rtl_magics[] = {
	"RTL\0", "RTC\0", "RTR\0",
	"RXL\0", "RXC\0"
};

typedef struct rottmap {
	uint32_t used;
	uint32_t crc;
	uint32_t tag;
	uint32_t flags;
	uint32_t ofs_planes[3];
	uint32_t len_planes[3];
	char name[24];
} rottmap_t;

typedef struct rottmapset {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	vfsfile_t *handle;
	rottmap_t maps[100];
	void *mutex;
	int references;
} rottmapset_t;

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
	for (i = 0; i < pak->num_files; i++)
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
}

static unsigned int QDECL FSROTT_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	rottmap_t *file = (rottmap_t *)hashedresult;
	rottmapset_t *pak = (rottmapset_t *)handle;
	int i;

	if (file)
	{
		//is this a pointer to a file in this pak?
		if (file < pak->files || file > pak->files + pak->num_files)
			return FF_NOTFOUND;
	}
	else
	{
		for (i = 0; i < pak->num_files; i++)
		{
			if (strcasecmp(filename, pak->files[i].name) == 0)
			{
				file = &pak->files[i];
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
			loc->len = file->len;
		}

		return FF_FOUND;
	}

	return FF_NOTFOUND;
}

typedef struct {
	vfsfile_t funcs;
	rottmapset_t *parent;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfsrottmapset_t;

static int QDECL VFSROTT_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	int read = 0;

	if (bytestoread + vfsp->currentpos > vfsp->length)
		bytestoread = vfsp->length - vfsp->currentpos;

	if (Sys_LockMutex(vfsp->parent->mutex))
	{
		VFS_SEEK(vfsp->parent->handle, vfsp->startpos + vfsp->currentpos);
		read = VFS_READ(vfsp->parent->handle, buffer, bytestoread);
		vfsp->currentpos += read;
		Sys_UnlockMutex(vfsp->parent->mutex);
	}

	return read;
}

static int QDECL VFSROTT_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to ROTT mapset files\n");
	return 0;
}

static qofs_t QDECL VFSROTT_Tell(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->currentpos;
}

static qofs_t QDECL VFSROTT_GetLen(struct vfsfile_s *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	return vfsp->length;
}

static qboolean QDECL VFSROTT_Close(vfsfile_t *vfs)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	FSROTT_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSROTT_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsrottmapset_t *vfsp = (vfsrottmapset_t *)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;
	return true;
}

static vfsfile_t *QDECL FSROTT_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	rottmapset_t *pak = (rottmapset_t *)handle;
	vfsrottmapset_t *vfs;
	rottmap_t *f = (rottmap_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsrottmapset_t));

	vfs->parent = pak;
	if (!Sys_LockMutex(vfs->parent->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}

	vfs->parent->references++;
	Sys_UnlockMutex(vfs->parent->mutex);

	vfs->startpos = f->ofs;
	vfs->length = loc->len;
	vfs->currentpos = 0;

#ifdef _DEBUG
	Q_strlcpy(vfs->funcs.dbgname, f->name, sizeof(vfs->funcs.dbgname));
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

	for (i = 0; i < pak->num_files; i++)
	{
		if (filefuncs->WildCmp(match, pak->files[i].name))
		{
			if (!func(pak->files[i].name, pak->files[i].len, 0, parm, handle))
				return false;
		}
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
	char magic[4];
	uint32_t version;
	rottmapset_t *pak;
	int i, j;
	qboolean valid = false;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// validate magic
	if (VFS_READ(file, magic, sizeof(magic)) < sizeof(magic))
		return NULL;
	for (i = 0; i < ASIZE(rtl_magics); i++)
		if (memcmp(rtl_magics[i], magic, 4) == 0)
			valid = true;
	if (!valid)
		return NULL;

	// validate version
	if (VFS_READ(file, &version, sizeof(version)) < sizeof(version))
		return NULL;
	if (version != rtl_version && version != rxl_version)
		return NULL;

	// alloc
	pak = (rottmapset_t *)plugfuncs->Malloc(sizeof(*pak));

	// read maps
	if (VFS_READ(file, pak->maps, sizeof(pak->maps)) < sizeof(pak->maps))
		return NULL;

	// setup info
	pak->references++;
	pak->handle = file;
	strcpy(pak->desc, desc);
	VFS_SEEK(pak->handle, 0);
	pak->mutex = Sys_CreateMutex();

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
