#pragma once
#include "window.h"
#include <SDL2/SDL.h>

void init_imgui(Window *);
void destroy_imgui(void);
void render_window(Window *);
void renderer_handle_event(Window *, SDL_Event *);
