#include <SDL2/SDL.h>

#define MS_IN_S	1000


SDL_Window *window;
SDL_Renderer *renderer;
SDL_TimerID timer;


static inline void helper_initgfx(int width, int height)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_TIMER);
	SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);

	SDL_RenderPresent(renderer);
}

static inline void helper_deinitgfx()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

static inline void helper_set_draw_color(int r, int g, int b, int a)
{
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

static inline void helper_draw_point(int x, int y)
{
	SDL_RenderDrawPoint(renderer, x, y);
}

static inline void helper_clear_screen(int r, int g, int b, int a)
{
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderClear(renderer);
}

static inline void helper_update_screen(void)
{
	SDL_RenderPresent(renderer);
}

static inline int helper_is_key_pressed(int key)
{
	const uint8_t *state = SDL_GetKeyboardState(NULL);

#define K(c8k, sdlk)	[c8k] = SDL_SCANCODE_##sdlk,
	const int keys[0x10] = {
		K(0x1, 1) K(0x2, 2) K(0x3, 3) K(0xC, 4)
		K(0x4, Q) K(0x5, W) K(0x6, E) K(0xD, R)
		K(0x7, A) K(0x8, S) K(0x9, D) K(0xE, F)
		K(0xA, Z) K(0x0, X) K(0xB, C) K(0xF, V)
	};
#undef K

	if (key < 0x10)
		return state[keys[key]];
	return 0;

}

static inline int helper_is_user_exit(void)
{
	SDL_Event event;
	return SDL_PollEvent(&event) && event.type == SDL_QUIT;
}

static inline uint32_t helper_time(void)
{
	return SDL_GetTicks();
}

static inline void helper_sleep(uint32_t ms)
{
	SDL_Delay(ms);
}

void (*cb)(void);

uint32_t timer_callback(uint32_t ms, void *params)
{
	(void) params;
	cb();
	return ms;
}

static inline void helper_add_timer(uint32_t ms, void (*callback)(void))
{
	cb = callback;
	timer = SDL_AddTimer(ms, timer_callback, NULL);
}

static inline void helper_remove_timer(void)
{
	SDL_RemoveTimer(timer);
}

#ifdef WIN32
#include "windows.h"
static inline void helper_beep(void)
{
	MessageBeep(0);
}
#else
static inline void helper_beep(void)
{
	int _ = system("beep");
	(void) _;
}
#endif

