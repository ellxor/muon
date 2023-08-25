#pragma once

#include "bitboard.h"

// Bitboard tables to cache the generation of piece attacks. For all sliding piece moves, this uses
// about 1MB of storage, which can fit into the larger caches of newer CPUs. This can be reducded to
// about 210kb if using pdep masks, which could be more cache-efficient for a larger project such as a
// chess engine.

#define MAGIC_BITBASE_SIZE 107648
typedef struct { bitboard mask, *attacks; } magic;

bitboard knight_attacks[64];
bitboard   king_attacks[64];

bitboard line_between[64][64];
bitboard sliding_attacks[MAGIC_BITBASE_SIZE];

magic bishop_magics[64];
magic rook_magics[64];


bitboard bishop_attacks(square sq, bitboard occ) {
	magic m = bishop_magics[sq]; return m.attacks[pext(occ, m.mask)];
}


bitboard rook_attacks(square sq, bitboard occ) {
	magic m = rook_magics[sq]; return m.attacks[pext(occ, m.mask)];
}


// Generate diagonal for bishop moves, the diagonals are from bottom-left to top-right, with the
// main diagonal (index 0) being A1 to H8. The index (n) specifies the digonal, with positive
// shifting the digonal toward A8, and negative toward H1.

bitboard generate_diagonal(int n) {
	bitboard main_diag = 0x8040201008040201;
	return (n >= 0) ? main_diag << (8 * n) : main_diag >> -(8 * n);
}


bitboard generate_sliding_attacks(square sq, bitboard mask, bitboard occ)
{
	occ &= mask; // only use the occupancy of squares we need
	bitboard bit = 1ull << sq;

	bitboard lower = occ & (bit - 1);
	bitboard upper = occ ^ lower;

	lower = 0x8000000000000000u >> clz(lower | 1);   // isolate msb of lower bits...
	return mask & ((upper ^ (upper - lower)) ^ bit); // ... and extract range up to lsb of upper bits
}


//  Generate the line (diagonal or orthogonal) between two squares, used for pinned piece masks and
//  blocking checks. The mask returned does include the bit for square b, used to allow pieces to
//  capture a checking piece.

bitboard generate_line_between(square a, square b)
{
	bitboard a_bb = 1ull << a;
	bitboard b_bb = 1ull << b;

	bitboard diag = bishop_attacks(a, b_bb);
	bitboard orth = rook_attacks(a, b_bb);

	bitboard line = 0;

	if (diag & b_bb)  line = diag & bishop_attacks(b, a_bb);
	if (orth & b_bb)  line = orth & rook_attacks(b, a_bb);

	return line | b_bb;
}


void init_bitbase_tables()
{
	int index = 0;

	for (square sq = 0; sq < 64; sq += 1) {
		bitboard bit = 1ull << sq;

		knight_attacks[sq] = north(north(east(bit))) | north(north(west(bit)))
		                   | south(south(east(bit))) | south(south(west(bit)))
		                   | east(east(north(bit)))  | east(east(south(bit)))
		                   | west(west(north(bit)))  | west(west(south(bit)));

		king_attacks[sq] = north(bit) | east(bit) | south(bit) | west(bit)
		                 | north(east(bit)) | north(west(bit)) | south(east(bit)) | south(west(bit));


		// bishop attacks
		{
			square file = sq & 7, rank = sq >> 3;

			bitboard diag = generate_diagonal(rank - file);
			bitboard anti = bswap(generate_diagonal(7 - rank - file));

			// Clear outer bits of mask. These not needed for magic bitboards as a
			// sliding piece can always move to the edge of the board if the square
			// just before is unoccupied. We also clear the bit of the square as this
			// is always occupied by the moving piece itself so is irrelevant.

			bitboard outer = AFILE | HFILE | RANK1 | RANK8 | bit;
			bitboard mask = (diag | anti) &~ outer;

			bishop_magics[sq].mask = mask;
			bishop_magics[sq].attacks = sliding_attacks + index;
			bitboard occ = 0;

			do {
				sliding_attacks[index++] = generate_sliding_attacks(sq, diag, occ)
				                         | generate_sliding_attacks(sq, anti, occ);
				occ = (occ - mask) & mask; // iterate over all subset bitboards of a bitboard
			}
			while (occ);
		}

		// rook attacks
		{
			bitboard file = AFILE << (sq &  7);
			bitboard rank = RANK1 << (sq & 56);

			// Rook moves are generated using the same techniques as bishop moves
			// above, except more care must be taken with the board edges.

			const bitboard file_outer = RANK1 | RANK8;
			const bitboard rank_outer = AFILE | HFILE;

			bitboard mask = ((file &~ file_outer) | (rank &~ rank_outer)) &~ bit;

			rook_magics[sq].mask = mask;
			rook_magics[sq].attacks = sliding_attacks + index;

			bitboard occ = 0;

			do {
				sliding_attacks[index++] = generate_sliding_attacks(sq, file, occ)
				                         | generate_sliding_attacks(sq, rank, occ);
				occ = (occ - mask) & mask;
			}
			while (occ);
		}
	}

	// line_between is generated separately after as it relies on bishop and rook moves to already be generated.
	for (square a = 0; a < 64; a += 1)
		for (square b = 0; b < 64; b += 1) line_between[a][b] = generate_line_between(a,b);
}
