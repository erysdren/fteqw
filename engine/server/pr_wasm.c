/*
 * written by erysdren (it/its)
 */

#include "quakedef.h"

#ifdef VM_WASM

#include "pr_common.h"
#include "hash.h"

#include "wasm_export.h"

#define STACK_SIZE 8192
#define HEAP_SIZE 8192

static struct wasm {
	pubprogfuncs_t progfuncs;
	progexterns_t progfuncsparms;
	edict_t **edicttable;
	unsigned int maxedicts;
	void *programdata;
	qofs_t len_programdata;
	unsigned int numfunctiontable;
	struct wasmfunctiontable {
		char name[256];
		wasm_function_inst_t func;
	} *functiontable;
	RuntimeInitArgs init_args;
	wasm_module_t module;
	wasm_module_inst_t module_inst;
	wasm_exec_env_t exec_env;
} wasm;

static void QDECL WASM_CloseProgs(pubprogfuncs_t *inst)
{
	free(wasm.edicttable);
	free(wasm.functiontable);
	wasm_runtime_destroy_exec_env(wasm.exec_env);
	wasm_runtime_deinstantiate(wasm.module_inst);
	wasm_runtime_unload(wasm.module);
	FS_FreeFile(wasm.programdata);

	memset(&wasm, 0, sizeof(wasm));
}

static char *QDECL WASM_AddString(pubprogfuncs_t *pf, const char *val, int minlength, pbool demarkup)
{
	return NULL;
}

static edict_t *QDECL WASM_EdictNum(pubprogfuncs_t *pf, unsigned int num)
{
	int newcount;
	if (num >= wasm.maxedicts)
	{
		newcount = num + 64;
		wasm.edicttable = realloc(wasm.edicttable, newcount * sizeof(*wasm.edicttable));
		while (wasm.maxedicts < newcount)
			wasm.edicttable[wasm.maxedicts++] = NULL;

		pf->edicttable_length = wasm.maxedicts;
		pf->edicttable = wasm.edicttable;
	}
	return wasm.edicttable[num];
}

static unsigned int QDECL WASM_NumForEdict(pubprogfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}

static int QDECL WASM_EdictToProgs(pubprogfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}

static edict_t *QDECL WASM_ProgsToEdict(pubprogfuncs_t *pf, int num)
{
	return WASM_EdictNum(pf, num);
}

static edict_t *QDECL WASM_CreateEdict(unsigned int num)
{
	edict_t *e;
	e = wasm.edicttable[num] = Z_Malloc(sizeof(edict_t));
	wasm_runtime_module_malloc(wasm.module_inst, sv.world.edict_size, (void **)&e->v);
#ifdef VM_Q1
	e->xv = (extentvars_t *)(e->v + 1);
#endif
	e->entnum = num;
	return e;
}

static void QDECL WASM_EntClear(pubprogfuncs_t *pf, edict_t *e)
{
	memset(e->v, 0, sv.world.edict_size);
	e->ereftype = ER_ENTITY;
}

static edict_t *QDECL WASM_DoRespawn(pubprogfuncs_t *pf, edict_t *e, int num)
{
	if (!e)
		e = WASM_CreateEdict(num);
	else
		WASM_EntClear(pf, e);
	ED_Spawned((struct edict_s *)e, false);
	return e;
}

static edict_t *QDECL WASM_EntAlloc(pubprogfuncs_t *pf, pbool isobject, size_t extrasize)
{
	int i;
	edict_t *e;
	for (i = 0; i < sv.world.num_edicts; i++)
	{
		e = (edict_t *)EDICT_NUM_PB(pf, i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e || (ED_ISFREE(e) && (e->freetime < 2 || sv.time - e->freetime > 0.5)))
		{
			e = WASM_DoRespawn(pf, e, i);
			return (struct edict_s *)e;
		}
	}

	if (i >= sv.world.max_edicts - 1) //try again, but use timed out ents.
	{
		for (i = 0; i < sv.world.num_edicts; i++)
		{
			e = (edict_t *)EDICT_NUM_PB(pf, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || ED_ISFREE(e))
			{
				e = WASM_DoRespawn(pf, e, i);
				return (struct edict_s *)e;
			}
		}

		if (i >= sv.world.max_edicts-1)
		{
			Sys_Error("ED_Alloc: no free edicts");
		}
	}

	sv.world.num_edicts++;
	e = WASM_EdictNum(pf, i);

	e = WASM_DoRespawn(pf, e, i);

	return (struct edict_s *)e;
}

static void QDECL WASM_EntRemove(pubprogfuncs_t *pf, edict_t *e, qboolean instant)
{
	if (!ED_CanFree(e))
		return;

	e->ereftype = ER_FREE;
	e->freetime = instant ? 0 : sv.time; //can respawn instantly when asked.
}

static eval_t *QDECL WASM_FindGlobal(pubprogfuncs_t *pf, const char *name, progsnum_t num, etype_t *type)
{
	/* TODO */
	return NULL;
}

static qboolean QDECL WASM_Event_ContentsTransition(world_t *w, wedict_t *ent, int oldwatertype, int newwatertype)
{
	/* do legacy behaviour (whatever that means) */
	return false;
}

static void QDECL WASM_Get_FrameState(world_t *w, wedict_t *ent, framestate_t *fstate)
{
	memset(fstate, 0, sizeof(*fstate));
	fstate->g[FS_REG].frame[0] = ent->v->frame;
	fstate->g[FS_REG].frametime[0] = ent->xv->frame1time;
	fstate->g[FS_REG].lerpweight[0] = 1;
	fstate->g[FS_REG].endbone = 0x7fffffff;

	fstate->g[FST_BASE].frame[0] = ent->xv->baseframe;
	fstate->g[FST_BASE].frametime[0] = ent->xv->/*base*/frame1time;
	fstate->g[FST_BASE].lerpweight[0] = 1;
	fstate->g[FST_BASE].endbone = ent->xv->basebone;

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	if (ent->xv->skeletonindex)
		skel_lookup(w, ent->xv->skeletonindex, fstate);
#endif
}

static void QDECL WASM_ExecuteProgram(pubprogfuncs_t *pf, func_t func)
{
	if (func == 0)
		return;

	if (func & (1 << 31))
	{
		/* high bit set, so it was looked up by name */
		func &= ~(1 << 31);
		func -= 1;
		wasm_runtime_call_wasm(wasm.exec_env, wasm.functiontable[func].func, 0, NULL);
	}
	else
	{
		/* high bit not set, so it's a direct wasm function index */
		func -= 1;
		wasm_runtime_call_indirect(wasm.exec_env, func, 0, NULL);
	}
}

static func_t QDECL WASM_FindFunction(pubprogfuncs_t *pf, const char *name, progsnum_t num)
{
	/* TODO: hashtable? */
	int i;
	wasm_function_inst_t func;

	/* check cache */
	/* return index+1 since 0 means null */
	/* set the high bit so we can resolve it properly later */
	for (i = 0; i < wasm.numfunctiontable; i++)
		if (strncmp(name, wasm.functiontable[i].name, sizeof(wasm.functiontable[i].name)) == 0)
			return ((func_t)(i + 1) | (1 << 31));

	/* lookup function */
	func = wasm_runtime_lookup_function(wasm.module_inst, name);
	if (!func)
		return 0;

	/* add to cache */
	wasm.functiontable = realloc(wasm.functiontable, (wasm.numfunctiontable + 1) * sizeof(struct wasmfunctiontable));

	/* clear cache entry */
	memset(&wasm.functiontable[wasm.numfunctiontable], 0, sizeof(struct wasmfunctiontable));

	/* copy in cached function */
	strncpy(wasm.functiontable[wasm.numfunctiontable].name, name, sizeof(wasm.functiontable[wasm.numfunctiontable].name));
	wasm.functiontable[wasm.numfunctiontable].func = func;

	/* return index+1 since 0 means null */
	wasm.numfunctiontable += 1;
	return ((func_t)(wasm.numfunctiontable) | (1 << 31));
}

static int QDECL WASM_LoadEnts(
	pubprogfuncs_t *pf,
	const char *mapstring,
	void *ctx,
	void (QDECL *memoryreset)(pubprogfuncs_t *pf, void *ctx),
	void (QDECL *entspawned)(pubprogfuncs_t *pf, struct edict_s *ed, void *ctx, const char *entstart, const char *entend),
	pbool(QDECL *extendedterm)(pubprogfuncs_t *pf, void *ctx, const char **extline)
)
{
	/* TODO */
	return sv.world.edict_size;
}

static void WASM_SetupGlobals(world_t *world)
{
	/* TODO */
}

qboolean PR_LoadWASM(void)
{
	char error_buf[256];
	pubprogfuncs_t *pf;

	/* load source file */
	if ((wasm.len_programdata = FS_LoadFile("qwprogs.wasm", &wasm.programdata)) != (qofs_t)-1)
		progstype = PROG_QW;
	else if ((wasm.len_programdata = FS_LoadFile("progs.wasm", &wasm.programdata)) != (qofs_t)-1)
		progstype = PROG_NQ;
	else
		return false;

	/* setup arguments */
	wasm.init_args.mem_alloc_type = Alloc_With_System_Allocator;

	/* initialize runtime */
	if (!wasm_runtime_full_init(&wasm.init_args))
		return false;

	/* load module */
	wasm.module = wasm_runtime_load(wasm.programdata, wasm.len_programdata, error_buf, sizeof(error_buf));
	if (!wasm.module)
		return false;

	/* instantiate module */
	wasm.module_inst = wasm_runtime_instantiate(wasm.module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
	if (!wasm.module_inst)
		return false;

	/* create execution environment */
	wasm.exec_env = wasm_runtime_create_exec_env(wasm.module_inst, STACK_SIZE);
	if (!wasm.exec_env)
		return false;

	pf = svprogfuncs = &wasm.progfuncs;

	pf->Shutdown = WASM_CloseProgs;
	pf->AddString = WASM_AddString;
	pf->EdictNum = WASM_EdictNum;
	pf->NumForEdict = WASM_NumForEdict;
	pf->EdictToProgs = WASM_EdictToProgs;
	pf->ProgsToEdict = WASM_ProgsToEdict;
	pf->EntAlloc = WASM_EntAlloc;
	pf->EntFree = WASM_EntRemove;
	pf->EntClear = WASM_EntClear;
	pf->FindGlobal = WASM_FindGlobal;
	pf->load_ents = WASM_LoadEnts;
	pf->GetEdictFieldValue = WASM_GetEdictFieldValue;
	pf->SetStringField = WASM_SetStringField;
	pf->StringToProgs = WASM_StringToProgs;
	pf->StringToNative = WASM_StringToNative;
	pf->ExecuteProgram = WASM_ExecuteProgram;
	pf->FindFunction = WASM_FindFunction;

	sv.world.Event_Touch = WASM_Event_Touch;
	sv.world.Event_Think = WASM_Event_Think;
	sv.world.Event_Sound = SVQ1_StartSound;
	sv.world.Event_ContentsTransition = WASM_Event_ContentsTransition;
	sv.world.Get_CModel = SVPR_GetCModel;
	sv.world.Get_FrameState = WASM_Get_FrameState;

	sv.world.progs = pf;
	sv.world.progs->parms = &wasm.progfuncsparms;
	sv.world.progs->parms->user = &sv.world;
	sv.world.progs->parms->Printf = PR_Printf;
	sv.world.progs->parms->DPrintf = PR_DPrintf;
	sv.world.usesolidcorpse = true;

	WASM_SetupGlobals(world);

	/* TODO: why? */
	svs.numprogs = 0;

#ifdef VM_Q1
	sv.world.edict_size = sizeof(stdentvars_t) + sizeof(extentvars_t);
#else
	sv.world.edict_size = sizeof(stdentvars_t);
#endif

	/* TODO: why? */
	sv.haveitems2 = true;

	/* spawn the world */
	sv.world.edicts = (wedict_t *)pf->EntAlloc(pf, false, 0);

	/* clean up */
	return true;
}

#endif	/* VM_WASM */
