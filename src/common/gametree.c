#include "gametree.h"
#include <assert.h>
#include <stdio.h>

/* ================================================================================================
 * GAMETREE IMPLEMENTATION
 * ================================================================================================
 *
 * TODO: add an explanation of what we are doing.
  */

typedef uint8_t move_index;
typedef uint16_t variation_index;

constexpr variation_index variation_nullptr = UINT16_MAX;
constexpr uint16_t MAX_VARIATION_LENGTH = 32;

typedef struct GameVariation  GameVariation;
typedef struct GameTree       GameTree;


struct GameVariation
{
	variation_index next_variation,
			prev_variation,
			parent_variation;

	uint16_t branch_index   : 6,
		 branch_length  : 6,
		 branch_extends : 1;

	uint32_t sub_variation_mask;
	move_index moves[MAX_VARIATION_LENGTH];
};


/* TODO: gametree structure info goes here...
 */

constexpr size_t MAX_VARIATION_COUNT = 1024;
constexpr size_t MAX_GAME_PLY = 512;


struct GameTree
{
	// TODO: eventually add the option of using dynamic memory to expand the game tree if the
	// number of variations is not enough.
	GameVariation variation_table[MAX_VARIATION_COUNT];

	GameVariation *current_variation;
	move_index current_move_index;

	// TODO: use dynamic memory if stack is too small
	size_t ply;
	Board  board_stack[MAX_GAME_PLY];
	move   move_stack[MAX_GAME_PLY];

	// TODO: use a better form of memory management. This should include a bitset of which
	// indices are in use, as we want to reuse the space occupied by deleted variations.
	variation_index next_free_index;
};


/* Initial game tree state: */

static GameTree state =
{
	.variation_table = {[0] = { .next_variation   = variation_nullptr,
	                            .prev_variation   = variation_nullptr,
	                            .parent_variation = variation_nullptr,
	                          }},

	.current_variation   = &state.variation_table[0],
	.current_move_index  = 0,
	.board_stack	     = {BOARD_STARTPOS},
	.move_stack          = {{0}},
	.ply                 = 0,
	.next_free_index     = 1,
};



/* ================================================================================================
 *  VARIATION SEARCHER
 * ================================================================================================
 */


static GameVariation *search_needle;
static uint16_t search_branch_index;
static Board search_board;
static MoveBuffer search_moves;
static move_index search_index;


void initialise_variation_search()
{
	search_needle = state.current_variation;
	search_branch_index = state.current_move_index;

	search_board = state.board_stack[state.ply];
	search_moves = generate_moves(search_board);
}


move *get_next_variation_from_search()
{
	while (search_needle->next_variation != variation_nullptr)
	{
		search_needle = &state.variation_table[search_needle->next_variation];

		if (search_needle->branch_index == search_branch_index)
		{
			assert(search_needle->branch_length > 0 && "variation must have moves!");
			move_index index = search_needle->moves[0];

			search_index = index;
			return &search_moves.buffer[index];
		}
	}

	return nullptr;
}


void select_variation_from_search()
{
	state.current_variation = search_needle;
	state.current_move_index = 1;

	Board board = state.board_stack[state.ply];
	move move = search_moves.buffer[search_index];
	make_move(&board, move);

	state.ply += 1;
	state.board_stack[state.ply] = board;
	state.move_stack[state.ply] = move;
}


/* ================================================================================================
 *  VARIATION MEMORY MANGAGEMENT
 * ================================================================================================
 */


static inline
GameVariation *alloc_variation(move_index branch_index, move_index move)
{
	// TODO: use better memory management if the variation table is full
	assert(state.next_free_index < MAX_VARIATION_COUNT && "variation table is full!");

	variation_index index  = state.next_free_index++;
	variation_index parent = state.current_variation - state.variation_table;
	variation_index child  = state.current_variation->next_variation;

	if (child != variation_nullptr)
		state.variation_table[child].prev_variation = index;

	/* Create an initialise new variation
	 */
	GameVariation *variation = &state.variation_table[index];

	variation->prev_variation   = variation_nullptr;
	variation->next_variation   = child;
	variation->parent_variation = parent;

	variation->branch_index     = branch_index;
	variation->branch_length    = 1;
	variation->branch_extends   = false;

	variation->sub_variation_mask = 0x0;
	variation->moves[0] = move;

	return variation;
}


static inline
void dealloc_variation(variation_index index)
{
	(void) index;
	assert(false && "dealloc_variation() is unimplemented!");	
}



/* ================================================================================================
 *  VARIATION OPERATIONS
 * ================================================================================================
 */


static inline
void variation_append_move(move_index move)
{
	assert(state.current_move_index == state.current_variation->branch_length &&
		"we must be at end end of the variation to append a move!");

	/* If the current variation is full then we have to allocate and append a new variation
	 * buffer.
	 */
	if (state.current_variation->branch_length == MAX_VARIATION_LENGTH)
	{
		state.current_variation = alloc_variation(MAX_VARIATION_LENGTH, move);
		state.current_move_index = 0;
	}

	else {
		state.current_variation->moves[state.current_variation->branch_length++] = move;
	}
}


static inline
void variation_insert_move(move_index move)
{
	assert(state.current_move_index < state.current_variation->branch_length &&
		"use variation_append_move() instead to append moves to end of branch!");

	/* First we check to see if the move being inserted is already equal to the move that
	 * exists on the current branch
	 */
	move_index existing = state.current_variation->moves[state.current_move_index];

	if (move == existing)
		return;

	/* Next we check to see if the move being inserted is equal to any of the subvariations
	 * that exist for the current branch. We use the sub-variation bitmask to shortcut this
	 * if no subvariations exist.
	 */
	if ((state.current_variation->sub_variation_mask >> state.current_move_index) & 1)
	{
		initialise_variation_search();

		while (get_next_variation_from_search() != nullptr)
		{
			if (search_index == move)
			{
				state.current_variation = search_needle;	
				state.current_move_index = 0;
				return;
			}
		}
	}

	/* Finally, if the move does not already exist in the game tree then we allocate a new
	 * sub-variation.
	 */
	state.current_variation = alloc_variation(state.current_move_index, move);
	state.current_move_index = 0;
}


/* ================================================================================================
 *  PUBLIC GAMETREE OPERATIONS
 * ================================================================================================
 */


Board gametree_get_board()
{
	return state.board_stack[state.ply];
}


move *gametree_get_last_move()
{
	return (state.ply == 0) ? nullptr : &state.move_stack[state.ply];
}


bool gametree_make_move(move search)
{
	/* Lookup move in the movebuffer to get the index of the move (used for compression
	 * reasons.
	 */
	move_index index = 0;

	Board position = state.board_stack[state.ply];
	MoveBuffer moves = generate_moves(position);

	for (index = 0; index < moves.count; index += 1)
	{
		move candidate = moves.buffer[index];

		if (candidate.init == search.init && candidate.dest == search.dest)
			break;
	}


	/* If there are no candidate moves matching the search, then the function fails
	 */
	if (index == moves.count)
		return false;


	/* If a piece is specified by the search, then we assume the piece is the choice of a pawn
	 * promotion. As pawn promotions are generated contiguously, we can simply increment the
	 * index by a calculated offset. Note: it is a little dangerous to assume that this is how
	 * the movegen will always work, but I don't plan on changing it any time soon and we make
	 * such assumptions sparingly in the codebase so bugs shouldn't be too difficult to track
	 * down.
	 */
	if (search.piece != NONE) {
		assert(extract_piece(position, search.init) == PAWN && (search.dest >> 3) == 7
			&& "move.piece should be NONE if the move is not a promotion!");

		index += search.piece - KNIGHT;
	}


	/* Select whether or not to append or insert a move
	 */
	printf("Append [0] or Insert [1] mode: %d\n", state.current_move_index == state.current_variation->branch_length);
	(state.current_move_index == state.current_variation->branch_length)
		? variation_append_move(index)
		: variation_insert_move(index);


	/* Update and push the board to the board/move stack. Also increment the state variables.
	 */
	move move = moves.buffer[index];
	make_move(&position, move);

	state.ply += 1;
	state.current_move_index += 1;

	state.board_stack[state.ply] = position;
	state.move_stack[state.ply] = move;

	return true;
}


bool gametree_undo_move(void)
{
	/* First check if we are at the root of the game tree, and then pop the last board and move
	 * off of the stack, we don't need to actually clear the memory.
	 */
	if (state.ply == 0)
		return false;

	/* Finally, we need to decrement the current move index for the current variation. We also
	 * check if we are at the start of a variation, and traverse to the parent variation if
	 * necessary
	 */

	if (state.current_move_index == 0) {
		variation_index parent = state.current_variation->parent_variation;
		assert(parent != variation_nullptr && "variation must have a parent variation!");

		state.current_move_index = state.current_variation->branch_index;
		state.current_variation = &state.variation_table[parent];
	}

	state.current_move_index -= 1;
	state.ply -= 1;

	return true;
}


bool gametree_redo_move(void)
{
	/* First, we check if we are at the end of the current variation buffer, and if so then we
	 * try to jump to the next variation.
	 */
	if (state.current_move_index == MAX_VARIATION_LENGTH)
	{
		initialise_variation_search();

		if (get_next_variation_from_search() != nullptr)
		{
			state.current_variation = search_needle;
			state.current_move_index = 0;

			assert(state.current_variation->branch_length != 0
				&& "variation branch cannot be empty!");
		}
	}

	/* Next we check to see if we are at the end of the current variation. If so, then we
	 * early return as there are no more moves to make! Note: this case can never happen for
	 * a chained variation covered by the if statement above, as an empty variation should never
	 * exist.
	 */
	if (state.current_move_index == state.current_variation->branch_length)
		return false;

	/* Finally make the move in the gametree. */
	Board board = state.board_stack[state.ply];
	MoveBuffer moves = generate_moves(board);

	move move = moves.buffer[state.current_variation->moves[state.current_move_index]];
	make_move(&board, move);

	state.current_move_index += 1;
	state.ply += 1;

	state.board_stack[state.ply] = board;
	state.move_stack[state.ply] = move;

	return true;
}

