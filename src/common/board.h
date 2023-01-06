#pragma once
#include "bitboard.h"

/* The piecetypes are as follows, note: castle represents a rook that can still be castled with
*/
typedef enum : uint8_t { NONE, PAWN, KNIGHT, BISHOP, CASTLE, ROOK, QUEEN, KING } piecetype;

/* The position is stored in 4 compressed bitboards. Note: each piece defined above is made up of 3
 * bits, which will be referred to as [xyz] with x being the least significant bit. The nth bit in
 * each of the bitboards, x y and z corresponds to the piece occupying the nth square on the board
 * (A1-H8). For example, in the start position, C1 is the third square which is occupied by a bishop
 * (value = 3, XYZ = 110), so the 3rd bit will be set in the x and y bitboards, and clear in the z
 * bitboard.
 *
 * The position is stored rotated from the perspective of the side to move (which is always
 * considered to be white). The 'white' bitboard stores where all of the friendly pieces are. It
 * also stores a 1 on a square where en-passant is possible. The castling rights are embedded into
 * the board by the 'castle' piece type, which decays to a rook when moved, or if the king moves.
 */
typedef struct { bitboard x, y, z, white; } Board;

static inline
Board startpos(void)
{
	constexpr Board board = {
		.x = 0x34FF00000000FF34,
		.y = 0x7E0000000000007E,
		.z = 0x9900000000000099,
		.white = 0xFFFF,
	};

	return board;
}

static inline
bitboard occupied(Board board) {
	return board.x | board.y | board.z;
}


static inline
bitboard extract(Board board, piecetype piece) {
	if (piece == ROOK)
		return board.z &~ board.y; // castles are also rooks

	/* this looks slow and inefficient but in most cases `piece` is a compile-time constant
	 * so this code can be efficiently folded by the compiler
	 */
	return ((piece & 1) ? board.x :~ board.x)
	     & ((piece & 2) ? board.y :~ board.y)
	     & ((piece & 4) ? board.z :~ board.z);
}


static inline
piecetype extract_piece(Board board, square sq)
{
	return (piecetype)((((board.x >> sq) & 1) << 0)
	                 | (((board.y >> sq) & 1) << 1)
	                 | (((board.z >> sq) & 1) << 2));
}


/* Place a piece on a given square of the board. Note: this implementation assumes the square is
 * empty, so must be cleared if previously occupuied. It also assumes the piece is friendly (white).
 */
static inline
void set_square(Board *board, square sq, piecetype piece)
{
	board->x |= (((bitboard)piece >> 0) & 1) << sq;
        board->y |= (((bitboard)piece >> 1) & 1) << sq;
        board->z |= (((bitboard)piece >> 2) & 1) << sq;
        board->white |= 1ull << sq;
}

