#pragma once
#include <common/board.h>
#include <common/movegen.h>
#include <SDL2/SDL.h>


/* Window structure: we store a copy of the board, the game tree as well as some handles to some
 * SDL resources.
 */
typedef struct
{
	Board         board;
	bool          true_white;
	square        selected;

	move          last_move;
	bool          had_last_move;

	MoveBuffer    legal_moves;
	bool          legal_moves_generated;

	SDL_Renderer *renderer;
	SDL_Window   *frame;
}
Window;

constexpr square SQUARE_NONE = 0xFF;

Window *init_window(const char *window_title);
void start_window_loop(Window *);
void destroy_window(Window *);

