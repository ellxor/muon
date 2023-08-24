#pragma once

#include <stdbool.h>
#include "board.h"

/// TODO: this file is in need of cleaning up. It is just a basic implementation that works for now.
//  This parser will be integrated with other parsers, such as moves and UCI options.


const piecetype piece_lookup[128] = {
	['p'] = PAWN,
	['n'] = KNIGHT,
	['b'] = BISHOP,
	['r'] = ROOK,
	['q'] = QUEEN,
	['k'] = KING,
};


// Parse Forsyth-Edwards Notation for a legal chess position.
//   (Reference: https://www.chessprogramming.org/Forsyth-Edwards_Notation)

board parse_fen(const char *fen_string, bool *white_to_move, bool *ok)
{
	board board = {0};
	square sq = 56, file = 0;

	/* Parse board */
	while (sq != 8 || file != 8)
	{
		const char c = *fen_string++;
		const char lower_mask = 0x20;

		if (file > 8) goto error;

		/* end of rank */
		if (file == 8) {
			if (c != '/') goto error;
			sq += S+S, file = 0;
			continue;
		}

		/* blank squares */
		if ('1' <= c && c <= '8') {
			square offset =	c - '0';
			sq += offset, file += offset;
		}

		else {
			piecetype piece = piece_lookup[c | lower_mask];
			if (piece == NONE) goto error;

			/* Check if piece is black */
			set_square(&board, sq, piece);
			if (c & lower_mask) board.white ^= 1ull << sq;

			sq += 1, file += 1;
		}
	}

	/* space separator */
	if (*fen_string++ != ' ') goto error;

	/* parse side-to-move */
	switch (*fen_string++) {
		case 'w': *white_to_move = true; break;
		case 'b': *white_to_move = false; break;
		default : goto error;
	}

	/* space separator */
	if (*fen_string++ != ' ') goto error;

	/* parse castling rights */
	enum { A8 = 56, H8 = 63 };

	if (*fen_string == '-')
		fen_string += 1;

	else while (*fen_string != ' ') {
		bitboard castling_mask = 0;

		switch (*fen_string++) {
			case 'K': castling_mask |= 1ull << H1; break;
			case 'Q': castling_mask |= 1ull << A1; break;
			case 'k': castling_mask |= 1ull << H8; break;
			case 'q': castling_mask |= 1ull << A8; break;
			default : goto error;
		}

		/* flip rooks to castles */
		board.x ^= castling_mask;
	}

	/* space separator */
	if (*fen_string++ != ' ') goto error;

	/* parse en-passant */
	bitboard en_passant_mask = 0;

	if (*fen_string != '-') {
		square file = *fen_string++ - 'a';
		square rank = *fen_string++ - '1';

		if (file >= 8 || rank >= 8) goto error;

		square en_passant = (rank << 3) + file;
		en_passant_mask = 1ull << en_passant;
	}

	/* There is more FEN info after this point such as movenumber and 50 move clock but we don't
	 * care about this information yet. TODO: handle this information
	 */

	/* Rotate bitboards if black is the side to move */
	if (*white_to_move)
		board.white |= en_passant_mask;

	else {
		bitboard black = occupied(board) &~ board.white;

		board.x = bswap(board.x);
		board.y = bswap(board.y);
		board.z = bswap(board.z);
		board.white = bswap(black | en_passant_mask);
	}

	*ok = true;
	return board;

error:
	*ok = false;
	return board;
}
