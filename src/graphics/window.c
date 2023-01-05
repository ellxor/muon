#include "renderer.h"
#include "window.h"

#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>


static inline
void log_fatal_error(const char *msg)
{
	fprintf(stderr, "error :: %s: %s\n", msg, SDL_GetError());
	exit(EXIT_FAILURE);
}


/* Create a new window context. If any SDL setup functions we exit the program as a GUI is pretty
 * useless without a usable window.
 */
Window *init_window(const char *title)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		log_fatal_error("could not initalise SDL");

	/* Magic to get better image rendering */
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

	/* Default window config: */
	constexpr int INIT_WIDTH = 600;
	constexpr int INIT_HEIGHT = 600;

	constexpr uint32_t window_flags = SDL_WINDOW_RESIZABLE
	                                | SDL_WINDOW_ALLOW_HIGHDPI;

	constexpr uint32_t renderer_flags = SDL_RENDERER_ACCELERATED
	                                  | SDL_RENDERER_PRESENTVSYNC;

	static Window window;

	window.frame = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                                INIT_WIDTH, INIT_HEIGHT, window_flags);

	if (!window.frame) log_fatal_error("could not create SDL window");

	window.renderer = SDL_CreateRenderer(window.frame, -1, renderer_flags);
	if (!window.renderer) log_fatal_error("could not create SDL renderer");

	window.board = startpos();
	window.true_white = true;
	window.selected = SQUARE_NONE;
	return &window;
}


/* Clean up the window when we exit the program. The OS will clean-up the memory regardless but we
 * might as well do the proper thing.
 */
void destroy_window(Window *window)
{
	SDL_DestroyRenderer(window->renderer);
	SDL_DestroyWindow(window->frame);
	SDL_Quit();
}


/* Run main window loop indefinitely until the window is closed. This controls the main logic flow
 * of the program and controls the renderer and passes events around to where they need to be dealt
 * with.
 */
void start_window_loop(Window *window)
{
	bool quit = false;

	while (!quit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			renderer_handle_event(&event);

			quit = (event.type == SDL_QUIT) ||
			      ((event.type == SDL_WINDOWEVENT &&
                                event.window.event == SDL_WINDOWEVENT_CLOSE &&
			        event.window.windowID == SDL_GetWindowID(window->frame)));
		}

		render_window(window);	
	}
}

