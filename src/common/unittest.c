#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "board.h"
#include "movegen.h"
#include <parser/fen.h>

/* Unit-testing structure containing an FEN, and the (maximum) depth, as well as a list of expected
 * perft results at a given depth
 */
typedef struct { const char *name, *FEN; unsigned depth; size_t expected[7]; } UnitTest;


/* Unit-test results we obtained from (https://www.chessprogramming.org/Perft_Results)
 */
static UnitTest unit_tests[] =
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

constexpr size_t count_unit_tests = sizeof unit_tests / sizeof unit_tests[0];


static
size_t perft(Board board, unsigned depth)
{
	MoveBuffer moves = generate_moves(board);

	if (depth == 1) // shortcut leaf nodes
		return moves.count;

	size_t total = 0;

	for (size_t i = 0; i < moves.count; i += 1) {
		Board child = board;
		make_move(&child, moves.buffer[i]);
		total += perft(child, depth - 1);
	}

	return total;
}


int main()
{
	init_bitbase_tables();

	clock_t ticks = 0;
	size_t total_nodes = 0;

	printf("\n[Start of unit tests]\n");

	for (size_t index = 0; index < count_unit_tests; index += 1)
	{
		UnitTest test = unit_tests[index];
		printf("\nRunning unit test %s (\"%s\"):\n", test.name, test.FEN);

		bool white_to_move, ok;
		Board board = parse_fen(test.FEN, &white_to_move, &ok);

		assert(ok && "FEN parsing failed!");

		for (unsigned depth = 1; depth <= test.depth; depth += 1) {
			ticks -= clock();
			size_t nodes = perft(board, depth);
			ticks += clock();

			printf("  depth %u: %zu\n", depth, nodes);

			size_t expected = test.expected[depth-1];

			if (nodes != expected) {
				printf("    [TEST FAILED] expected %zu\n", expected);
				assert(0);
			}

			total_nodes += nodes;
		}
	}

	/* Calculate time for printing out benchmark */
	constexpr double tick_seconds = 1.0 / CLOCKS_PER_SEC;
	double seconds = ticks * tick_seconds;

	printf("\nNodes per second: %.0f\n\n", total_nodes / seconds);
	printf("[End of unit tests]\n\n");
}

