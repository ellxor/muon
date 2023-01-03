#include <stdio.h>
#include <time.h>
#include "common/board.h"
#include "common/movegen.h"


static
size_t perft(Board board, unsigned depth)
{
	MoveBuffer moves = generate_moves(board);

	if (depth == 1)
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

	Board board = startpos();

	clock_t t1 = clock();
	size_t nodes = perft(board, 6);
	clock_t t2 = clock();

	double seconds = (double)(t2 - t1) / CLOCKS_PER_SEC;

	printf("Nodes: %zu\n", nodes);
	printf("Speed: %.0f mnps\n", nodes / seconds / 1e6);
}

