#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

#include "board.h"
#include "fen.h"
#include "movegen.h"


//  Unit-testing structure containing an FEN, and the (maximum) depth, as well as a list of expected
//  perft results at a given depth

typedef struct { const char *name, *FEN; unsigned depth; size_t expected[7]; } unittest;

// Unit-test results we obtained from (https://www.chessprogramming.org/Perft_Results)
const unittest unit_tests[] =
{
	{ .name = "startpos",
	  .FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	  .depth = 6,
	  .expected = { 20, 400, 8902, 197281, 4865609, 119060324 },
	},

	{ .name = "kiwipete",
	  .FEN = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
	  .depth = 5,
	  .expected = { 48, 2039, 97862, 4085603, 193690690 },
	},

	{ .name = "tricky en-passant",
	  .FEN = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
	  .depth = 7,
	  .expected = { 14, 191, 2812, 43238, 674624, 11030083, 178633661 },
	},

	{ .name = "tricky castling",
	  .FEN = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -",
	  .depth = 6,
	  .expected = { 6, 264, 9467, 422333, 15833292, 706045033 },
	},

	{ .name = "tricky castling rotated",
	  .FEN = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -",
	  .depth = 6,
	  .expected = { 6, 264, 9467, 422333, 15833292, 706045033 },
	},

	{ .name = "talkchess",
	  .FEN = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
	  .depth = 5,
	  .expected = { 44, 1486, 62379, 2103487, 89941194 },
	},

	{ .name = "normal middlegame",
	  .FEN = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -",
	  .depth = 5,
	  .expected = { 46, 2079, 89890, 3894594, 164075551 },
	},
};

const size_t count_unit_tests = sizeof unit_tests / sizeof unit_tests[0];


size_t perft(board pos, unsigned depth)
{
	movebuffer moves = generate_moves(pos);
	if (depth == 1) return moves.count + popcnt(moves.pawn_push);

	size_t total = 0;

	for (size_t i = 0; i < moves.count; i += 1) {
		board child = make_move(pos, moves.buffer[i]);
		total += perft(child, depth - 1);
	}

	for bits(moves.pawn_push) {
		board child = make_pawn_push(pos, ctz(moves.pawn_push));
		total += perft(child, depth - 1);
	}

	return total;
}


int main()
{
	init_bitbase_tables();
	setlocale(LC_NUMERIC, "");

	clock_t ticks = 0;
	size_t total_nodes = 0;

	printf("name                      depth       nodes    \n");
	printf("===============================================\n");

	for (size_t index = 0; index < count_unit_tests; index += 1)
	{
		unittest test = unit_tests[index];

		bool white_to_move, ok;
		board board = parse_fen(test.FEN, &white_to_move, &ok);
		assert(ok && "FEN parsing failed!");

		clock_t t1 = clock();
		size_t nodes = perft(board, test.depth);
		clock_t t2 = clock();

		total_nodes += nodes;
		ticks += t2 - t1;

		printf("%-25s %-5u       %9zu\t\t(%zu mnps)\n", test.name, test.depth, nodes, nodes * CLOCKS_PER_SEC / (t2 - t1) / 1000000);

		size_t expected = test.expected[test.depth-1];
		assert(nodes == expected && "TEST FAILED!");
	}

	// Calculate time for printing out benchmark
	double tick_seconds = 1.0 / CLOCKS_PER_SEC;
	double seconds = ticks * tick_seconds;

	printf("\nNodes per second: %'d\n", (int)(total_nodes / seconds));
}
