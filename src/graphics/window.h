#pragma once
#include <common/board.h>
#include <common/movegen.h>
#include <SDL2/SDL.h>


/* Window structure: we store a copy of the board, the game tree as well as some handles to some
 * SDL resources.
 */
typedef struct
{
	bool   true_white;
	square selected;

	SDL_Renderer *renderer;
	SDL_Window   *frame;
}
Window;

constexpr square SQUARE_NONE = 0xFF;

Window *init_window(const char *window_title);
void start_window_loop(Window *);
void destroy_window(Window *);

