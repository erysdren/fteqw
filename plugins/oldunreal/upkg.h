// note i've kept this file light on FTE specific stuff so it can be copied out
// and used elsewhere --erysdren
#ifndef _UPKG_H_
#define _UPKG_H_

#include "../plugin.h"

#define UPKG_MAGIC "\xC1\x83\x2A\x9E"

#define UPKG_MALLOC(sz) plugfuncs->Malloc(sz)
#define UPKG_FREE(ptr) plugfuncs->Free(ptr)
#define UPKG_FILE vfsfile_t*
#define UPKG_FILE_READ(fil, buf, sz) VFS_READ(fil, buf, sz)
#define UPKG_FILE_SEEK(fil, ofs) VFS_SEEK(fil, ofs)

#pragma pack(push)
typedef struct upkg_generation {
	int32_t num_exports;
	int32_t num_names;
} upkg_generation_t;

typedef struct upkg_name {
	uint8_t len_name;
	char name[UINT8_MAX];
	uint32_t flags;
} upkg_name_t;

typedef struct upkg_export {
	int32_t class_index; // compact index
	int32_t super_index; // compact index
	int32_t package_index;
	int32_t object_name; // compact index
	uint32_t object_flags;
	int32_t len_serial; // compact index
	int32_t ofs_serial; // compact index
} upkg_export_t;

typedef struct upkg_import {
	int32_t class_package; // compact index
	int32_t class_name; // compact index
	int32_t package_index;
	int32_t object_name; // compact index
} upkg_import_t;

typedef struct upkg_header {
	uint32_t magic;
	uint16_t format_version;
	uint16_t licensee_version;
	uint32_t package_flags;
	int32_t num_names;
	int32_t ofs_names;
	int32_t num_exports;
	int32_t ofs_exports;
	int32_t num_imports;
	int32_t ofs_imports;
	uint32_t guid[4];
	int32_t num_generations;
	// upkg_generation_t generations[num_generations];
	// upkg_name_t names[num_names];
	// upkg_export_t exports[num_exports];
	// upkg_import_t imports[num_imports];
} upkg_header_t;

typedef struct upkg {
	upkg_header_t header;
	upkg_generation_t *generations;
	upkg_name_t *names;
	upkg_export_t *exports;
	upkg_import_t *imports;
} upkg_t;
#pragma pack(pop)

// read upkg from a file pointer
upkg_t *upkg_read_file(UPKG_FILE file);

// read upkg from a memory buffer
upkg_t *upkg_read_mem(uint8_t *buf, size_t buflen);

// free upkg
void upkg_free(upkg_t *upkg);

// read an unreal engine compact index from a given file
// returns the index in the out pointer
void upkg_read_compact_index_file(UPKG_FILE file, int32_t *out);

// read an unreal engine compact index from a given pointer
// returns the new position in the buffer after reading
// returns the unpacked compact index in the out pointer
uint8_t *upkg_read_compact_index_mem(uint8_t *ptr, int32_t *out);

#endif // _UPKG_H_
