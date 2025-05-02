#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"
#include <SDL2/SDL.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_Surface *screen;
static SDL_Surface *backbuffersurf;

static unsigned int *backbuffer;
static unsigned int *depthbuffer;
static unsigned int framenumber;

qboolean SW_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	int bpp = info->bpp;
	vid.pixelwidth = info->width;
	vid.pixelheight = info->height;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
		SDL_InitSubSystem(SDL_INIT_VIDEO);

	if (bpp != 32)
		bpp = 32; //sw renderer supports only this

	backbuffer = BZ_Malloc(vid.pixelwidth * vid.pixelheight * sizeof(*backbuffer));
	if (!backbuffer)
		return false;
	depthbuffer = BZ_Malloc(vid.pixelwidth * vid.pixelheight * sizeof(*depthbuffer));
	if (!depthbuffer)
		return false;

	window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, vid.pixelwidth, vid.pixelheight, SDL_WINDOW_RESIZABLE);
	SDL_SetWindowMinimumSize(window, vid.pixelwidth, vid.pixelheight);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(renderer, vid.pixelwidth, vid.pixelheight);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	screen = SDL_CreateRGBSurfaceWithFormatFrom(NULL, vid.pixelwidth, vid.pixelheight, 0, 0, SDL_PIXELFORMAT_ARGB8888);
	backbuffersurf = SDL_CreateRGBSurfaceWithFormatFrom(backbuffer, vid.pixelwidth, vid.pixelheight, 0, vid.pixelwidth*4, SDL_PIXELFORMAT_ARGB8888);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, vid.pixelwidth, vid.pixelheight);

	return true;
}
void SW_VID_DeInit(void)
{
	BZ_Free(backbuffer);
	backbuffer = NULL;

	BZ_Free(depthbuffer);
	depthbuffer = NULL;

	SDL_FreeSurface(screen);
	screen = NULL;

	SDL_FreeSurface(backbuffersurf);
	backbuffersurf = NULL;

	SDL_DestroyTexture(texture);
	texture = NULL;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
qboolean SW_VID_ApplyGammaRamps		(unsigned int rampcount, unsigned short *ramps)
{
	return false;
}
char *SW_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	void *ret = BZ_Malloc(vid.pixelwidth*vid.pixelheight*4);
	if (!ret)
		return NULL;

	memcpy(ret, backbuffer, vid.pixelwidth*vid.pixelheight*4);
	*bytestride = vid.pixelwidth*4;
	*truevidwidth = vid.pixelwidth;
	*truevidheight = vid.pixelheight;
	*fmt = TF_BGRX32;
	return ret;
}
void SW_VID_SetWindowCaption(const char *msg)
{
	SDL_SetWindowTitle(window, msg);
}
void SW_VID_SwapBuffers(void)
{
	if (SDL_LockTexture(texture, NULL, &screen->pixels, &screen->pitch) == 0)
	{
		SDL_Rect r = {0, 0, vid.pixelwidth, vid.pixelheight};
		SDL_LowerBlit(backbuffersurf, &r, screen, &r);
		SDL_UnlockTexture(texture);
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopyEx(renderer, texture, NULL, NULL, 0, NULL, SDL_FLIP_VERTICAL);
	SDL_RenderPresent(renderer);

	framenumber++;
}
void SW_VID_UpdateViewport(wqcom_t *com)
{
	com->viewport.cbuf = backbuffer;
	com->viewport.dbuf = depthbuffer;
	com->viewport.width = vid.pixelwidth;
	com->viewport.height = vid.pixelheight;
	com->viewport.stride = vid.pixelwidth;	//this is in pixels. which is stupid.
	com->viewport.framenum = framenumber;
}

#endif
