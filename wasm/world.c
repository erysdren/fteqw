
#include "defs.h"

int framecount = 0;

void WASM_EXPORT(StartFrame)(void)
{
	framecount++;
}
