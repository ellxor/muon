#pragma once

#include "bitboard.h"

typedef int piecetype;
enum piecetype { NONE, PAWN, KNIGHT, BISHOP, CASTLE, ROOK, QUEEN, KING };


// The position is stored in 4 compressed bitboards. Each piecetype defined above is made up of 3
// bits, which will be referred to as [xyz] with x being the least significant bit. The nth bit in
// each of the bitboards, x y and z corresponds to the piece occupying the nth square on the board.
//
// The position is stored rotated from the perspective of the side to move (which is always
// considered to be white). The 'white' bitboard stores where all of the friendly pieces are. It
// also stores a 1 on a square where en-passant is possible. The castling rights are embedded into
// the board by the 'castle' piece type, which decays to a rook when moved, or if the king moves.

typedef struct { bitboard x, y, z, white; } board;

// chess starting position, FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
#define BOARD_STARTPOS ((board) { 0x34ff00000000ff34, 0x7e0000000000007e, 0x9900000000000099, 0xffff })


bitboard occupied(board board) {
	return board.x | board.y | board.z;
}


bitboard extract(board board, piecetype const piece)
{
	if (piece == ROOK)
		return board.z &~ board.y; // castles are also rooks

	// this looks slow and inefficient but `piece` is a compile-time constant
	// so this code can be efficiently folded by the compiler
	return ((piece & 0x1) ? board.x : ~board.x)
	     & ((piece & 0x2) ? board.y : ~board.y)
	     & ((piece & 0x4) ? board.z : ~board.z);
}


//  Place a piece on a given square of the board. Note: this implementation assumes the square is
//  empty, so must be cleared if previously occupuied. It also assumes the piece is friendly (white).

void set_square(board *board, square sq, piecetype piece)
{
	bitboard bit = 1ull << sq;
	board->white |= bit;

	if (piece & 0x1) board->x |= bit;
	if (piece & 0x2) board->y |= bit;
	if (piece & 0x4) board->z |= bit;
}
