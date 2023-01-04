#pragma once
#include <common/board.h>
#include <SDL2/SDL.h>

/* Window structure: we store a copy of the board, the game tree as well as some handles to some
 * SDL resources.
 */
typedef struct { Board         board;
                 SDL_Renderer *renderer;
	         SDL_Window   *frame;
} Window;


Window *init_window(const char *window_title);
void start_window_loop(Window *);
void destroy_window(Window *);

