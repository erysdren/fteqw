#include "../plugin.h"
#include "../../engine/common/fs.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

#define SECTOR_SIZE 2048

typedef struct gta3direntry {
	uint32_t ofs; // in sectors
	uint32_t len; // in sectors
	char name[24];
} gta3direntry_t;

typedef struct gta3dirfile {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	size_t ofs;
	size_t len;
} gta3dirfile_t;

typedef struct gta3dir {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	size_t handlepos;
	vfsfile_t *handle;
	size_t num_files;
	gta3dirfile_t *files;
	void *mutex;
	int references;
} gta3dir_t;

static void QDECL FSGTA3_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	gta3dir_t *pak = (gta3dir_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSGTA3_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	gta3dir_t *pak = (gta3dir_t *)handle;

	if (!Sys_LockMutex(pak->mutex))
		return; //ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return; //not free yet

	VFS_CLOSE(pak->handle);

	Sys_DestroyMutex(pak->mutex);

	plugfuncs->Free(pak->files);
	plugfuncs->Free(pak);
}

static void QDECL FSGTA3_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	gta3dir_t *pak = (gta3dir_t *)handle;
	int i;
	for (i = 0; i < pak->num_files; i++)
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
}

static unsigned int QDECL FSGTA3_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	gta3dirfile_t *file = (gta3dirfile_t *)hashedresult;
	gta3dir_t *pak = (gta3dir_t *)handle;
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

typedef struct vfsimg {
	vfsfile_t funcs;
	gta3dir_t *parent;
	qofs_t startpos;
	qofs_t length;
	qofs_t currentpos;
} vfsgta3dir_t;

static int QDECL VFSGTA3_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsgta3dir_t *vfsp = (vfsgta3dir_t *)vfs;
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

static int QDECL VFSGTA3_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to img files\n");
	return 0;
}

static qofs_t QDECL VFSGTA3_Tell(struct vfsfile_s *vfs)
{
	vfsgta3dir_t *vfsp = (vfsgta3dir_t *)vfs;
	return vfsp->currentpos;
}

static qofs_t QDECL VFSGTA3_GetLen(struct vfsfile_s *vfs)
{
	vfsgta3dir_t *vfsp = (vfsgta3dir_t *)vfs;
	return vfsp->length;
}

static qboolean QDECL VFSGTA3_Close(vfsfile_t *vfs)
{
	vfsgta3dir_t *vfsp = (vfsgta3dir_t *)vfs;
	FSGTA3_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSGTA3_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsgta3dir_t *vfsp = (vfsgta3dir_t *)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;
	return true;
}

static vfsfile_t *QDECL FSGTA3_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	gta3dir_t *pak = (gta3dir_t *)handle;
	vfsgta3dir_t *vfs;
	gta3dirfile_t *f = (gta3dirfile_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsgta3dir_t));

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
	vfs->funcs.Close = VFSGTA3_Close;
	vfs->funcs.GetLen = VFSGTA3_GetLen;
	vfs->funcs.ReadBytes = VFSGTA3_ReadBytes;
	vfs->funcs.Seek = VFSGTA3_Seek;
	vfs->funcs.Tell = VFSGTA3_Tell;
	vfs->funcs.WriteBytes = VFSGTA3_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSGTA3_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSGTA3_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSGTA3_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	gta3dir_t *pak = (gta3dir_t *)handle;
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

static int QDECL FSGTA3_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	gta3dir_t *pak = (gta3dir_t *)handle;
	return filefuncs->BlockChecksum(pak->files, sizeof(*pak->files) * pak->num_files);
}

static searchpathfuncs_t *QDECL FSGTA3_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	gta3dir_t *pak;
	vfsfile_t *img;
	size_t len_dir;
	char imgfilename[MAX_OSPATH];
	const char *dot;
	int i;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// read img file
	Q_strlcpy(imgfilename, filename, sizeof(imgfilename));
	if ((dot = strrchr(imgfilename, '.')))
	{
		Q_snprintf((char *)dot, sizeof(imgfilename) - (dot - imgfilename), ".img");
	}
	else
	{
		Q_strlcat(imgfilename, ".img", sizeof(imgfilename));
	}
	img = filefuncs->OpenVFS(imgfilename, "rb", FS_GAME);
	if (!img)
	{
		Con_Printf("gta3: failed to open \"%s\"\n", imgfilename);
		return NULL;
	}

	// get length of dir file
	len_dir = VFS_GETLEN(file);
	if (len_dir % sizeof(gta3direntry_t))
		Con_Printf("gta3: \"%s\" is possibly malformed\n", filename);

	// alloc
	pak = (gta3dir_t *)plugfuncs->Malloc(sizeof(*pak));
	pak->num_files = len_dir / sizeof(gta3direntry_t);
	pak->files = plugfuncs->Malloc(sizeof(*pak->files) * pak->num_files);

	// parse the files
	for (i = 0; i < pak->num_files; i++)
	{
		gta3direntry_t entry;
		if (VFS_READ(file, &entry, sizeof(entry)) < sizeof(entry))
			break;
		pak->files[i].ofs = LittleLong(entry.ofs) * SECTOR_SIZE;
		pak->files[i].len = LittleLong(entry.len) * SECTOR_SIZE;
		Q_strlncpy(pak->files[i].name, entry.name, sizeof(pak->files[i].name), sizeof(entry.name));
	}

	// we don't need the dir file anymore so clean it up
	VFS_CLOSE(file);

	// setup info
	pak->references++;
	pak->handle = img;
	pak->handlepos = 0;
	Q_strlcpy(pak->desc, desc, sizeof(pak->desc));
	VFS_SEEK(pak->handle, pak->handlepos);
	pak->mutex = Sys_CreateMutex();

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSGTA3_GetPathDetails;
	pak->pub.ClosePath = FSGTA3_ClosePath;
	pak->pub.BuildHash = FSGTA3_BuildHash;
	pak->pub.FindFile = FSGTA3_FindFile;
	pak->pub.ReadFile = FSGTA3_ReadFile;
	pak->pub.EnumerateFiles = FSGTA3_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSGTA3_GeneratePureCRC;
	pak->pub.OpenVFS = FSGTA3_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

qboolean Plug_FSGTA3_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_dir", FSGTA3_LoadArchive))
	{
		Con_Printf("engine doesn't support filesystem plugins\n");
		return false;
	}

	return true;
}
