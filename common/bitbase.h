#pragma once
#include "bitboard.h"
#include "board.h"

/* These functions use lookup tables to quickly get the possible moves for a given piece and square.
 * The sliding pieces, bishop and rook (and queen) use an occupancy bitboard as well to determine
 * movement, which can be efficiently converted to a lookup-index using a multiplication-shift or
 * 'pext' instruction if available.
 *
 * The line_between and line_connecting bitboards are useful to generate pinned piece masks, as well
 * as the moves for pinned pieces.
 */

bitboard knight_moves(square);
bitboard bishop_moves(square, bitboard occ);
bitboard   rook_moves(square, bitboard occ);
bitboard  queen_moves(square, bitboard occ);
bitboard   king_moves(square);

bitboard get_line_between(square, square);
bitboard get_line_connecting(square, square);

/* This function must be called once, before any of the functions above.
 */
void init_bitbase_tables();
