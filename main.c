#include <common/bitbase.h>
#include <graphics/renderer.h>
#include <graphics/window.h>

//static
//size_t perft(Board board, unsigned depth)
//{
//	MoveBuffer moves = generate_moves(board);
//
//	if (depth == 1)
//		return moves.count;
//
//	size_t total = 0;
//
//	for (size_t i = 0; i < moves.count; i += 1) {
//		Board child = board;
//		make_move(&child, moves.buffer[i]);
//		total += perft(child, depth - 1);
//	}
//
//	return total;
//}


int main()
{
	//init_bitbase_tables();

	Window *window = init_window("Muon pre-alpha version");
	init_imgui(window);

	start_window_loop(window);

	destroy_imgui();
	destroy_window(window);

}

