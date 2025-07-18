// note i've kept this file light on FTE specific stuff so it can be copied out
// and used elsewhere --erysdren
#include "upkg.h"

upkg_t *upkg_read_file(UPKG_FILE file)
{
	upkg_t *upkg;
	upkg_header_t header;

	UPKG_FILE_READ(file, &header, sizeof(header));

	if (memcmp(&header.magic, UPKG_MAGIC, 4) != 0)
		return NULL;

	// allocate and copy header
	upkg = UPKG_MALLOC(sizeof(*upkg));
	memcpy(&upkg->header, &header, sizeof(header));

	// allocate stuff
	upkg->generations = UPKG_MALLOC(upkg->header.num_generations * sizeof(*upkg->generations));
	upkg->names = UPKG_MALLOC(upkg->header.num_names * sizeof(*upkg->names));
	upkg->exports = UPKG_MALLOC(upkg->header.num_exports * sizeof(*upkg->exports));
	upkg->imports = UPKG_MALLOC(upkg->header.num_imports * sizeof(*upkg->imports));

	// read generations
	UPKG_FILE_READ(file, upkg->generations, upkg->header.num_generations * sizeof(*upkg->generations));

	// read names
	UPKG_FILE_SEEK(file, upkg->header.ofs_names);
	for (int32_t i = 0; i < upkg->header.num_names; i++)
	{
		UPKG_FILE_READ(file, &upkg->names[i].len_name, sizeof(upkg->names[i].len_name));
		UPKG_FILE_READ(file, upkg->names[i].name, upkg->names[i].len_name);
		upkg->names[i].name[upkg->names[i].len_name] = 0;
		UPKG_FILE_READ(file, &upkg->names[i].flags, sizeof(upkg->names[i].flags));
	}

	// read imports
	UPKG_FILE_SEEK(file, upkg->header.ofs_imports);
	for (int32_t i = 0; i < upkg->header.num_imports; i++)
	{
		upkg_read_compact_index_file(file, &upkg->imports[i].class_package);
		upkg_read_compact_index_file(file, &upkg->imports[i].class_name);
		UPKG_FILE_READ(file, &upkg->imports[i].package_index, sizeof(int32_t));
		upkg_read_compact_index_file(file, &upkg->imports[i].object_name);
	}

	// read exports
	UPKG_FILE_SEEK(file, upkg->header.ofs_exports);
	for (int32_t i = 0; i < upkg->header.num_exports; i++)
	{
		upkg_read_compact_index_file(file, &upkg->exports[i].class_index);
		upkg_read_compact_index_file(file, &upkg->exports[i].super_index);
		UPKG_FILE_READ(file, &upkg->exports[i].package_index, sizeof(int32_t));
		upkg_read_compact_index_file(file, &upkg->exports[i].object_name);
		UPKG_FILE_READ(file, &upkg->exports[i].object_flags, sizeof(int32_t));
		upkg_read_compact_index_file(file, &upkg->exports[i].len_serial);
		if (upkg->exports[i].len_serial > 0)
			upkg_read_compact_index_file(file, &upkg->exports[i].ofs_serial);
	}

	return upkg;
}

upkg_t *upkg_read_mem(uint8_t *buf, size_t buflen)
{
#define UPKG_MEM_READ(ptr, dst, len) memcpy(dst, ptr, len); ptr = (uint8_t *)(ptr) + (len)
	upkg_t *upkg;
	upkg_header_t *header = (upkg_header_t *)buf;
	uint8_t *ptr = buf;

	if (memcmp(&header->magic, UPKG_MAGIC, 4) != 0)
		return NULL;

	// allocate and copy header
	upkg = UPKG_MALLOC(sizeof(*upkg));
	memcpy(&upkg->header, header, sizeof(upkg->header));

	// allocate stuff
	upkg->generations = UPKG_MALLOC(upkg->header.num_generations * sizeof(*upkg->generations));
	upkg->names = UPKG_MALLOC(upkg->header.num_names * sizeof(*upkg->names));
	upkg->exports = UPKG_MALLOC(upkg->header.num_exports * sizeof(*upkg->exports));
	upkg->imports = UPKG_MALLOC(upkg->header.num_imports * sizeof(*upkg->imports));

	// read generations
	UPKG_MEM_READ(ptr, upkg->generations, upkg->header.num_generations * sizeof(*upkg->generations));

	// read names
	ptr = buf + upkg->header.ofs_names;
	for (int32_t i = 0; i < upkg->header.num_names; i++)
	{
		UPKG_MEM_READ(ptr, &upkg->names[i].len_name, sizeof(upkg->names[i].len_name));
		UPKG_MEM_READ(ptr, upkg->names[i].name, upkg->names[i].len_name);
		upkg->names[i].name[upkg->names[i].len_name] = 0;
		UPKG_MEM_READ(ptr, &upkg->names[i].flags, sizeof(upkg->names[i].flags));
	}

	// read imports
	ptr = buf + upkg->header.ofs_imports;
	for (int32_t i = 0; i < upkg->header.num_imports; i++)
	{
		ptr = upkg_read_compact_index_mem(ptr, &upkg->imports[i].class_package);
		ptr = upkg_read_compact_index_mem(ptr, &upkg->imports[i].class_name);
		UPKG_MEM_READ(ptr, &upkg->imports[i].package_index, sizeof(int32_t));
		ptr = upkg_read_compact_index_mem(ptr, &upkg->imports[i].object_name);
	}

	// read exports
	ptr = buf + upkg->header.ofs_exports;
	for (int32_t i = 0; i < upkg->header.num_exports; i++)
	{
		ptr = upkg_read_compact_index_mem(ptr, &upkg->exports[i].class_index);
		ptr = upkg_read_compact_index_mem(ptr, &upkg->exports[i].super_index);
		UPKG_MEM_READ(ptr, &upkg->exports[i].package_index, sizeof(int32_t));
		ptr = upkg_read_compact_index_mem(ptr, &upkg->exports[i].object_name);
		UPKG_MEM_READ(ptr, &upkg->exports[i].object_flags, sizeof(int32_t));
		ptr = upkg_read_compact_index_mem(ptr, &upkg->exports[i].len_serial);
		if (upkg->exports[i].len_serial > 0)
			ptr = upkg_read_compact_index_mem(ptr, &upkg->exports[i].ofs_serial);
	}

	return upkg;
#undef UPKG_MEM_READ
}

void upkg_free(upkg_t *upkg)
{
	if (upkg)
	{
		if (upkg->generations) UPKG_FREE(upkg->generations);
		if (upkg->names) UPKG_FREE(upkg->names);
		if (upkg->exports) UPKG_FREE(upkg->exports);
		if (upkg->imports) UPKG_FREE(upkg->imports);
		UPKG_FREE(upkg);
	}
}

#if 0
upkg_t *upkg_open(const char *filename)
{
	/* variables */
	int i;
	upkg_t *upkg;

	/* allocate struct */
	upkg = calloc(1, sizeof(upkg_t));

	/* alloc failed */
	if (upkg == NULL)
	{
		upkg_close(upkg);
		return NULL;
	}

	/* open file handle */
	upkg->handle = fopen(filename, "rb");
	if (upkg->handle == NULL)
	{
		upkg_close(upkg);
		return NULL;
	}

	/* read in header */
	fread(upkg, offsetof(upkg_t, generations), 1, upkg->handle);

	/* check if tag is correct */
	if (upkg->tag != 2653586369UL)
	{
		upkg_close(upkg);
		return NULL;
	}

	/* allocate generations */
	upkg->generations = calloc(upkg->num_generations, sizeof(upkg_generation_t));

	/* read in generations */
	fread(upkg->generations, sizeof(upkg_generation_t),
			upkg->num_generations, upkg->handle);

	/* allocate names */
	upkg->names = calloc(upkg->num_names, sizeof(upkg_name_t));

	/* read in names */
	fseek(upkg->handle, upkg->ofs_names, SEEK_SET);
	for (i = 0; i < upkg->num_names; i++)
	{
		fread(&upkg->names[i].len_name, sizeof(uint8_t), 1, upkg->handle);
		upkg->names[i].name = calloc(upkg->names[i].len_name, sizeof(char));
		fread(upkg->names[i].name, sizeof(char), upkg->names[i].len_name, upkg->handle);
		fread(&upkg->names[i].flags, sizeof(uint32_t), 1, upkg->handle);
	}

	/* allocate imports */
	upkg->imports = calloc(upkg->num_imports, sizeof(upkg_import_t));

	/* read in imports */
	fseek(upkg->handle, upkg->ofs_imports, SEEK_SET);
	for (i = 0; i < upkg->num_imports; i++)
	{
		upkg->imports[i].class_package = upkg_read_compact_index(upkg->handle);
		upkg->imports[i].class_name = upkg_read_compact_index(upkg->handle);
		fread(&upkg->imports[i].package_index, sizeof(int32_t), 1, upkg->handle);
		upkg->imports[i].object_name = upkg_read_compact_index(upkg->handle);
	}

	/* allocate exports */
	upkg->exports = calloc(upkg->num_exports, sizeof(upkg_export_t));

	/* read in exports */
	fseek(upkg->handle, upkg->ofs_exports, SEEK_SET);
	for (i = 0; i < upkg->num_exports; i++)
	{
		upkg->exports[i].class_index = upkg_read_compact_index(upkg->handle);
		upkg->exports[i].super_index = upkg_read_compact_index(upkg->handle);
		fread(&upkg->exports[i].package_index, sizeof(int32_t), 1, upkg->handle);
		upkg->exports[i].object_name = upkg_read_compact_index(upkg->handle);
		fread(&upkg->exports[i].object_flags, sizeof(uint32_t), 1, upkg->handle);
		upkg->exports[i].len_serial = upkg_read_compact_index(upkg->handle);

		if (upkg->exports[i].len_serial > 0)
			upkg->exports[i].ofs_serial = upkg_read_compact_index(upkg->handle);
	}

	return upkg;
}
#endif


// read an unreal engine compact index from a given file
// returns the index in the out pointer
void upkg_read_compact_index_file(UPKG_FILE file, int32_t *out)
{
	int32_t ret = 0;
	int sign = 1;

	for (int i = 0; i < 5; i++)
	{
		uint8_t x = 0;
		UPKG_FILE_READ(file, &x, sizeof(x));

		if (i == 0)
		{
			// first byte
			// bit: X0000000
			if ((x & 0x80) > 0)
				sign = -1;
			// bits: 00XXXXXX
			ret |= (x & 0x3F);
			// bit: 0X000000
			if ((x & 0x40) == 0)
				break;
		}
		else if (i == 4)
		{
			// last byte
			// bits: 000XXXXX -- the 0 bits are ignored (hits the 32 bit boundary)
			ret |= (x & 0x1F) << (6 + (3 * 7));
		}
		else
		{
			// middle byte
			// bits: 0XXXXXXX
			ret |= (x & 0x7F) << (6 + ((i - 1) * 7));
			// bit: X0000000
			if ((x & 0x80) == 0)
				break;
		}
	}

	// multiply by the sign
	ret *= sign;

	// return the unpacked integer
	if (out) *out = ret;
}

// read an unreal engine compact index from a given pointer
// returns the new position in the buffer after reading
// returns the index in the out pointer
uint8_t *upkg_read_compact_index_mem(uint8_t *ptr, int32_t *out)
{
	int32_t ret = 0;
	int sign = 1;

	for (int i = 0; i < 5; i++)
	{
		uint8_t x = *ptr++;

		if (i == 0)
		{
			// first byte
			// bit: X0000000
			if ((x & 0x80) > 0)
				sign = -1;
			// bits: 00XXXXXX
			ret |= (x & 0x3F);
			// bit: 0X000000
			if ((x & 0x40) == 0)
				break;
		}
		else if (i == 4)
		{
			// last byte
			// bits: 000XXXXX -- the 0 bits are ignored (hits the 32 bit boundary)
			ret |= (x & 0x1F) << (6 + (3 * 7));
		}
		else
		{
			// middle byte
			// bits: 0XXXXXXX
			ret |= (x & 0x7F) << (6 + ((i - 1) * 7));
			// bit: X0000000
			if ((x & 0x80) == 0)
				break;
		}
	}

	// multiply by the sign
	ret *= sign;

	// return the unpacked integer
	if (out) *out = ret;

	// return the new position in the buffer
	return ptr;
}
