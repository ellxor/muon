#pragma once
#include <common/board.h>
#include "window.h"

/* Offsets to get index for white or black piece
 */
constexpr size_t WHITE_OFFSET = 0x0;
constexpr size_t BLACK_OFFSET = 0x8;

/* Load piece texture from static memory. Index is constructed using white or black offset defined
 * above and the piecetype enum defined in common.
 */
SDL_Texture *load_texture(size_t index, Window *);

