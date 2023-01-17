#pragma once
#include "movegen.h"

/* TODO: add a brief description of how the gametree works here
 */


Board gametree_get_board(void);
move *gametree_get_last_move(void); // returns nullptr at root of game tree


/* Make and unmake moves on the game tree. These functions all return a bool which indicate if a
 * move was actually made or not. For instance, `make_move` will return false if the move prodided
 * has no legal candidate, and `undo_move` and `redo_move` will return false if there are no more
 * moves in the game tree (at the start and end).
 *
 * The move provided for `make_move` does not have to be a full legal move, but must provide the
 * initial and destination squares, and a piece should be provided in the case of pawn promotion,
 * otherwise a knight will be chosen by default.
 */
bool gametree_make_move(move);
bool gametree_undo_move(void);
bool gametree_redo_move(void);


/* TODO: add short explanation of how variation searching works. Maybe add a small code example?
 */
void  initialise_variation_search(void);
move *get_next_variation_from_search(void); // returns nullptr at end of search
void  select_variation_from_search(void);

