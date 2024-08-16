#pragma once

#include "bitboard.h"

// Bitboard tables to cache the generation of piece attacks. For all sliding piece moves, this uses
// about 1MB of storage, which can fit into the larger caches of newer CPUs. This can be reducded to
// about 210kb if using pdep masks, which could be more cache-efficient for a larger project such as a
// chess engine.

#define MAGIC_BITBASE_SIZE 107648
typedef struct { bitboard mask, magic, *attacks; } magic;

bitboard knight_attacks[64];
bitboard   king_attacks[64];

bitboard line_between[64][64];
bitboard sliding_attacks[MAGIC_BITBASE_SIZE];

magic bishop_magics[64];
magic rook_magics[64];


const bitboard _bishop_magics[64] = {
	0xE8400A008B120282, 0xED90240800802400, 0xEC041404005020A0, 0xEC28208620020000,
	0xEC140C206C0E00A8, 0xEF81112010000012, 0xEC02088220100222, 0xE8A100410403400A,
	0xEC00502481180200, 0xEC20040882040220, 0xEC20A10244004080, 0xEC900A0A12002008,
	0xEE8C621610000341, 0xEC00810120110800, 0xEC04891402209400, 0xEC00810088010800,
	0xEC6000086021AA10, 0xEE04180A88084100, 0xE4020824080A0408, 0xE604010840106000,
	0xE410808400A00044, 0xE421000205008202, 0xEC14282309011006, 0xEC00200201010820,
	0xEC484080200D0110, 0xEC05880060020400, 0xE404220094040400, 0xDC0228004C004108,
	0xDC00840082020201, 0xE408004012004200, 0xEC0A008000441000, 0xEE61014000A40402,
	0xEC02104023104A00, 0xEC450848C0606100, 0xE514040840040842, 0xDC000A0080480182,
	0xDD40008060020060, 0xE401100301008048, 0xEC82040051C40601, 0xEC41040104102100,
	0xEC0806101020C604, 0xEC0880841122209A, 0xE401008044423002, 0xE600A04202280800,
	0xE4096010A4000480, 0xE601101020408281, 0xEE08418302000C00, 0xEC440400424C0203,
	0xEC14040209142221, 0xEC00220104A00000, 0xEC22302084100040, 0xEC08003020A80080,
	0xEC00441002060204, 0xED00400448038080, 0xEC08100408840002, 0xEC20044100510041,
	0xEA08430400864080, 0xEF00404048041000, 0xEE24020600840480, 0xEC18010044940408,
	0xED80001040705101, 0xEC01102002020200, 0xEC0C226001010300, 0xE920200409006024,
};

const bitboard _rook_magics[64] = {
	0xD080062240009280, 0xD540013004402008, 0xD480200080100208, 0xD480049000800800,
	0xD500080017000410, 0xD600042806000110, 0xD4001003C2040108, 0xD080004220801100,
	0xD782002042028100, 0xD90A802000804000, 0xDA62801000802000, 0xDA0A0020C2001049,
	0xD862804401800800, 0xD801800200040080, 0xD812000822000304, 0xD401000041000092,
	0xD4400680004088A0, 0xD810808040012008, 0xDA20018010028020, 0xD908090021021000,
	0xD900910008010084, 0xD804008004800200, 0xD801040032211008, 0xD440020005004184,
	0xD480004040002010, 0xD801002300400080, 0xD8600101004010A1, 0xD800100100082300,
	0xD800850100180050, 0xD804000401104860, 0xD802000600698804, 0xD72001820020510C,
	0xD400400090800020, 0xD90A802000804000, 0xDA62801000802000, 0xDA0A0020C2001049,
	0xD800280081800400, 0xD804008004800200, 0xD812000822000304, 0xD401000041000092,
	0xD504400180208010, 0xD8500040A011C001, 0xD88100A000910040, 0xDA80681003010020,
	0xDA00440008008080, 0xD802002439460010, 0xD810080650040071, 0xD46A4C0050920001,
	0xD480062240009280, 0xD800C08208A10200, 0xD868128040220200, 0xD842210018100500,
	0xDA00440008008080, 0xD801800200040080, 0xD80041082A101400, 0xD4A210810C006A00,
	0xD013801022084102, 0xD58080C491020022, 0xD460A00852004082, 0xD400050061081001,
	0xD401000482102801, 0xD58D004400020821, 0xD602109042080104, 0xD028422048840102,
};

bitboard bishop_attacks(square sq, bitboard occ) {
	//magic m = bishop_magics[sq]; return m.attacks[pext(occ, m.mask)];
	magic m = bishop_magics[sq]; bitboard magic = _bishop_magics[sq]; return m.attacks[(occ & m.mask) * magic >> (magic >> 58)];
}


bitboard rook_attacks(square sq, bitboard occ) {
	//magic m = rook_magics[sq]; return m.attacks[pext(occ, m.mask)];
	magic m = rook_magics[sq]; bitboard magic = _rook_magics[sq]; return m.attacks[(occ & m.mask) * magic >> (magic >> 58)];
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
			bitboard magic = _bishop_magics[sq];

			bishop_magics[sq].mask = mask;
			bishop_magics[sq].attacks = sliding_attacks + index;
			index += 1 << popcnt(mask);

			bitboard occ = 0;

			do {
				size_t idx = (occ * magic) >> (magic >> 58);
				bishop_magics[sq].attacks[idx] = generate_sliding_attacks(sq, diag, occ)
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
			bitboard magic = _rook_magics[sq];

			rook_magics[sq].mask = mask;
			rook_magics[sq].attacks = sliding_attacks + index;
			index += 1 << popcnt(mask);

			bitboard occ = 0;

			do {
				size_t idx = (occ * magic) >> (magic >> 58);
				rook_magics[sq].attacks[idx] = generate_sliding_attacks(sq, file, occ)
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
