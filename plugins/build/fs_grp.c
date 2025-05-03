#include "../plugin.h"
#include "../../engine/common/fs.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

typedef struct dgrpfile {
	char name[12];
	uint32_t len;
} dgrpfile_t;

typedef struct grpfile {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	size_t ofs;
	size_t len;
} grpfile_t;

typedef struct grp {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	vfsfile_t *handle;
	size_t num_files;
	grpfile_t *files;
	void *mutex;
	dgrpfile_t *tree;
	int references;
} grp_t;

typedef struct {
	vfsfile_t funcs;
	grp_t *parent;
	qofs_t ofs;
	qofs_t len;
	qofs_t pos;
} vfsgrp_t;

static void QDECL FSGRP_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	grp_t *grp = (grp_t *)handle;
	*out = 0;
	if (grp->references != 1)
		Q_snprintfz(out, outlen, "(%i)", grp->references - 1);
}

static void QDECL FSGRP_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	grp_t *grp = (grp_t *)handle;

	if (!Sys_LockMutex(grp->mutex))
		return; //ohnoes
	stillopen = --grp->references > 0;
	Sys_UnlockMutex(grp->mutex);
	if (stillopen)
		return; //not free yet

	VFS_CLOSE(grp->handle);
	Sys_DestroyMutex(grp->mutex);
	plugfuncs->Free(grp->tree);
	plugfuncs->Free(grp->files);
	plugfuncs->Free(grp);
}

static void QDECL FSGRP_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	grp_t *grp = (grp_t *)handle;
	int i;
	for (i = 0; i < grp->num_files; i++)
		AddFileHash(depth, grp->files[i].name, &grp->files[i].bucket, &grp->files[i]);
}

static unsigned int QDECL FSGRP_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	grpfile_t *file = (grpfile_t *)hashedresult;
	grp_t *grp = (grp_t *)handle;
	int i;

	if (file)
	{
		//is this a pointer to a file in this pak?
		if (file < grp->files || file > grp->files + grp->num_files)
			return FF_NOTFOUND;
	}
	else
	{
		for (i = 0; i < grp->num_files; i++)
		{
			if (strcasecmp(filename, grp->files[i].name) == 0)
			{
				file = &grp->files[i];
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

static int QDECL VFSGRP_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsgrp_t *vfsp = (vfsgrp_t *)vfs;
	int read = 0;

	if (bytestoread + vfsp->pos > vfsp->len)
		bytestoread = vfsp->len - vfsp->pos;

	if (Sys_LockMutex(vfsp->parent->mutex))
	{
		VFS_SEEK(vfsp->parent->handle, vfsp->ofs + vfsp->pos);
		read = VFS_READ(vfsp->parent->handle, buffer, bytestoread);
		vfsp->pos += read;
		Sys_UnlockMutex(vfsp->parent->mutex);
	}

	return read;
}

static int QDECL VFSGRP_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to grp files\n");
	return 0;
}

static qofs_t QDECL VFSGRP_Tell(struct vfsfile_s *vfs)
{
	vfsgrp_t *vfsp = (vfsgrp_t *)vfs;
	return vfsp->pos;
}

static qofs_t QDECL VFSGRP_GetLen(struct vfsfile_s *vfs)
{
	vfsgrp_t *vfsp = (vfsgrp_t *)vfs;
	return vfsp->len;
}

static qboolean QDECL VFSGRP_Close(vfsfile_t *vfs)
{
	vfsgrp_t *vfsp = (vfsgrp_t *)vfs;
	FSGRP_ClosePath((searchpathfuncs_t *)vfsp->parent); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSGRP_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsgrp_t *vfsp = (vfsgrp_t *)vfs;
	if (pos > vfsp->len)
		return false;
	vfsp->pos = pos;
	return true;
}

static vfsfile_t *QDECL FSGRP_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	grp_t *grp = (grp_t *)handle;
	vfsgrp_t *vfs;
	grpfile_t *f = (grpfile_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsgrp_t));

	vfs->parent = grp;
	if (!Sys_LockMutex(vfs->parent->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}

	vfs->parent->references++;
	Sys_UnlockMutex(vfs->parent->mutex);

	vfs->ofs = f->ofs;
	vfs->len = loc->len;
	vfs->pos = 0;

#ifdef _DEBUG
	Q_strlcpy(vfs->funcs.dbgname, f->name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSGRP_Close;
	vfs->funcs.GetLen = VFSGRP_GetLen;
	vfs->funcs.ReadBytes = VFSGRP_ReadBytes;
	vfs->funcs.Seek = VFSGRP_Seek;
	vfs->funcs.Tell = VFSGRP_Tell;
	vfs->funcs.WriteBytes = VFSGRP_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSGRP_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSGRP_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSGRP_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	grp_t *grp = (grp_t *)handle;
	int i;

	for (i = 0; i < grp->num_files; i++)
	{
		if (filefuncs->WildCmp(match, grp->files[i].name))
		{
			if (!func(grp->files[i].name, grp->files[i].len, 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSGRP_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	grp_t *grp = (grp_t *)handle;
	return filefuncs->BlockChecksum(grp->tree, sizeof(*grp->tree) * grp->num_files);
}

static searchpathfuncs_t *QDECL FSGRP_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	char header[12];
	uint32_t num_files;
	uint32_t ofs;
	grp_t *grp;
	int i;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// read header
	if (VFS_READ(file, header, sizeof(header)) < sizeof(header))
		return NULL;

	// validate header
	if (memcmp(header, "KenSilverman", sizeof(header)) != 0)
		return NULL;

	// read number of files
	if (VFS_READ(file, &num_files, sizeof(num_files)) < sizeof(num_files))
		return NULL;

	// alloc
	grp = (grp_t *)plugfuncs->Malloc(sizeof(*grp));
	grp->tree = (dgrpfile_t *)plugfuncs->Malloc(sizeof(*grp->tree) * num_files);
	grp->files = (grpfile_t *)plugfuncs->Malloc(sizeof(*grp->files) * num_files);

	// read disk tree
	if (VFS_READ(file, grp->tree, sizeof(*grp->tree) * num_files) < sizeof(*grp->tree) * num_files)
	{
		plugfuncs->Free(grp);
		plugfuncs->Free(grp->tree);
		plugfuncs->Free(grp->files);
		return NULL;
	}

	// create our tree
	ofs = VFS_TELL(file);
	for (i = 0; i < num_files; i++)
	{
		memcpy(grp->files[i].name, grp->tree[i].name, 12);
		grp->files[i].name[12] = 0;
		grp->files[i].len = grp->tree[i].len;
		grp->files[i].ofs = ofs;
		ofs += grp->tree[i].len;
	}

	// setup info
	grp->num_files = num_files;
	grp->references++;
	grp->handle = file;
	strcpy(grp->desc, desc);
	VFS_SEEK(grp->handle, 0);
	grp->mutex = Sys_CreateMutex();

	// setup funcs
	grp->pub.fsver = FSVER;
	grp->pub.GetPathDetails = FSGRP_GetPathDetails;
	grp->pub.ClosePath = FSGRP_ClosePath;
	grp->pub.BuildHash = FSGRP_BuildHash;
	grp->pub.FindFile = FSGRP_FindFile;
	grp->pub.ReadFile = FSGRP_ReadFile;
	grp->pub.EnumerateFiles = FSGRP_EnumerateFiles;
	grp->pub.GeneratePureCRC = FSGRP_GeneratePureCRC;
	grp->pub.OpenVFS = FSGRP_OpenVFS;

	return (searchpathfuncs_t *)grp;
}

qboolean GRP_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	plugfuncs->ExportFunction("MayShutdown", NULL);
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_grp", FSGRP_LoadArchive))
	{
		Con_Printf("engine doesn't support filesystem plugins\n");
		return false;
	}

	return true;
}
