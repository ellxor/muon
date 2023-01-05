#include "textures.h"

#include <string.h>
#include <SDL2/SDL_image.h>

/* We currently use the incbin header to embed the piece graphics into the program. Eventually this
 * should be replaced by #embed in C23 once it becomes available in gcc/clang.
 */
#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin/incbin.h"

/* SVG graphics are actually text files, so we include them as such.
 * TODO: find which linker flags are necessary to include a path to these files, so that we can
 * use relative paths from this file.
 */
INCTXT(svg_white_pawn,   "../src/graphics/resources/wP.svg");
INCTXT(svg_white_knight, "../src/graphics/resources/wN.svg");
INCTXT(svg_white_bishop, "../src/graphics/resources/wB.svg");
INCTXT(svg_white_rook,   "../src/graphics/resources/wR.svg");
INCTXT(svg_white_queen,  "../src/graphics/resources/wQ.svg");
INCTXT(svg_white_king,   "../src/graphics/resources/wK.svg");

INCTXT(svg_black_pawn,   "../src/graphics/resources/bP.svg");
INCTXT(svg_black_knight, "../src/graphics/resources/bN.svg");
INCTXT(svg_black_bishop, "../src/graphics/resources/bB.svg");
INCTXT(svg_black_rook,   "../src/graphics/resources/bR.svg");
INCTXT(svg_black_queen,  "../src/graphics/resources/bQ.svg");
INCTXT(svg_black_king,   "../src/graphics/resources/bK.svg");

/* We bake the raw piece svg images into the executable in static memory
 */
typedef const void *SVG_Texture;

static
SVG_Texture piece_SVG[16] =
{
	[ WHITE_OFFSET + NONE   ] = nullptr,
	[ WHITE_OFFSET + PAWN   ] = svg_white_pawn_data,
	[ WHITE_OFFSET + KNIGHT ] = svg_white_knight_data,
	[ WHITE_OFFSET + BISHOP ] = svg_white_bishop_data,
	[ WHITE_OFFSET + CASTLE ] = svg_white_rook_data,
	[ WHITE_OFFSET + ROOK   ] = svg_white_rook_data,
	[ WHITE_OFFSET + QUEEN  ] = svg_white_queen_data,
	[ WHITE_OFFSET + KING   ] = svg_white_king_data,

	[ BLACK_OFFSET + NONE   ] = nullptr,
	[ BLACK_OFFSET + PAWN   ] = svg_black_pawn_data,
	[ BLACK_OFFSET + KNIGHT ] = svg_black_knight_data,
	[ BLACK_OFFSET + BISHOP ] = svg_black_bishop_data,
	[ BLACK_OFFSET + CASTLE ] = svg_black_rook_data,
	[ BLACK_OFFSET + ROOK   ] = svg_black_rook_data,
	[ BLACK_OFFSET + QUEEN  ] = svg_black_queen_data,
	[ BLACK_OFFSET + KING   ] = svg_black_king_data,
};

/* We also store a copy of precompiled SDL textures here to prevent reloading them, and I don't know
 * where else to stick them, without cluttering other files.
 */
typedef struct { SDL_Texture *texture; bool loaded; } PreCompiled_Texture;
static PreCompiled_Texture precompiled_textures[16];


SDL_Texture *load_texture(size_t index, Window *window)
{
	PreCompiled_Texture *precompiled = precompiled_textures + index;

	if (precompiled->loaded)
		return precompiled->texture;

	SVG_Texture texture = piece_SVG[index];

	// TODO: check that these pointers are not null
	SDL_RWops *rw = SDL_RWFromMem((char *)texture, strlen(texture));
	SDL_Surface *surface = IMG_Load_RW(rw, true);

	precompiled->texture = SDL_CreateTextureFromSurface(window->renderer, surface);
	precompiled->loaded = true;

	return precompiled->texture;
}

