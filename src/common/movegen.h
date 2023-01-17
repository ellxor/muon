#pragma once
#include "bitbase.h"
#include "bitboard.h"
#include "board.h"

/* Store a compressed move in 16 bits. The 'init' and 'dest' fields store the initial and
 * destination squares of the move, and the 'piece' field stores the piece that will occupy the
 * square at the end of the move (in case of promotion, the type of the promoted piece). There is a
 * single bit left over so we use this as a flag to indicate castling. This is redundant information
 * but it does yield a small performance improvement to the move generation.
 */
typedef struct { uint16_t init: 6, dest: 6, piece: 3, castling: 1; } move;


/* The generated moves are stored in a fixed-size buffer for performance, reallocations would slow
 * us down a lot. It is usually a large overallocation as chess has a branching factor of around
 * 30, but some positions, although exceedingly rare, do require this many moves.
 *
 * This position holds the record for the maximum number of possible legal moves at 218:
 *   fen: "3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1"
 */
constexpr size_t MAX_MOVES = 218;
typedef struct { size_t count; move buffer[MAX_MOVES]; } MoveBuffer;


static inline
void append_move(MoveBuffer *moves, move move) {
	moves->buffer[moves->count++] = move;
}


MoveBuffer generate_moves(Board);
void make_move(Board *, move);

