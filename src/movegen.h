#pragma once

#include "bitbase.h"
#include "bitboard.h"
#include "board.h"


//  Store a compressed move in 16 bits. The 'init' and 'dest' fields store the initial and
//  destination squares of the move, and the 'piece' field stores the piece that will occupy the
//  square at the end of the move (in case of promotion, the type of the promoted piece). There is a
//  single bit left over so we use this as a flag to indicate castling. This is redundant information
//  but it does yield a small performance improvement to the move generation. Making the move smaller
//  improves performance as the move buffer is relatively large (for the reasons give below), so it
//  helps to improve cache locality.

typedef uint16_t move;

#define M(init, dest, piece)	((init) | (dest) << 6 | (piece) << 13)
#define M_CASTLING		0x1000u

#define M_INIT(mov)   (((mov)      ) & 0x3f)
#define M_DEST(mov)   (((mov) >>  6) & 0x3f)
#define M_PIECE(mov)  ( (mov) >> 13)


//  The generated moves are stored in a fixed-size buffer for performance, reallocations would slow
//  us down a lot. It is usually a large overallocation as chess has a branching factor of around
//  30-40, but some positions, although exceedingly rare, do require this many moves.
//
//  This position holds the record for the maximum number of possible legal moves at 218:
//    FEN: 3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - -

#define MAX_MOVES 218
typedef struct { bitboard pawn_push; size_t count; move buffer[MAX_MOVES]; } movebuffer;

// some useful info to pass around to move generation
typedef struct { bitboard attacked, targets, en_passant, hpinned, vpinned; square king; } movegen_info;


void append_move(movebuffer *moves, move move) {
	moves->buffer[moves->count++] = move;
}


// Generate pawn moves from a move mask, from a given direction. This allows us to
// generate in more predictable loops.

void generate_partial_pawn_moves(movebuffer *buffer, bitboard moves, square direction, bool promotion)
{
	for bits(moves) {
		square dest = ctz(moves);
		square init = dest - direction;

		if (promotion) {
			append_move(buffer, M(init, dest, KNIGHT));
			append_move(buffer, M(init, dest, BISHOP));
			append_move(buffer, M(init, dest, ROOK));
			append_move(buffer, M(init, dest, QUEEN));
		}

		else {
			append_move(buffer, M(init, dest, PAWN));
		}
	}
}


// Generate pawn moves according to a targets mask (where pawns must end their move, for example
// in case of check this may be restricted), and a pinned mask which indicates pawns that are
// pinned to our king.

void generate_pawn_moves(movebuffer *buffer, movegen_info info, board board)
{
	bitboard pawns   = extract(board, PAWN) & board.white;
	bitboard occ     = occupied(board);
	bitboard enemy   = occ &~ board.white;
	bitboard targets = info.targets;

	// Check for pinned en-passant. Note that this is a special type of pinned piece as two
	// pieces dissappear in the checking direction. This introduces a slow branch into our pawn
	// move generation, but it is a necessary evil for full legality, however rare.

	bitboard candidates = pawns & south(east(info.en_passant) | west(info.en_passant));

	// We optimise this branch by only checking if the king is actually on the 5th rank
	if ((info.king & 56) == 32 && popcnt(candidates) == 1) {
		bitboard pinners = (extract(board, ROOK) | extract(board, QUEEN)) &~ board.white;
		bitboard clear = candidates | south(info.en_passant);

		// If the pawn is "double" pinned, then en-passant is no longer possible
		if (rook_attacks(info.king, (occ | info.en_passant) &~ clear) & pinners)
			info.en_passant = 0;
	}

	// enable en-passant if the pawn being captured was giving check
	targets |= info.en_passant & north(info.targets);
	enemy   |= info.en_passant;

	bitboard pinned = info.hpinned | info.vpinned;

	bitboard normal_pawns = pawns &~ pinned;
	bitboard pinned_pawns = pawns & pinned;

	bitboard file = AFILE << (info.king & 7); // only pinned pawns on same file as king can move forward
	bitboard forward = normal_pawns | (pinned_pawns & file);

	bitboard single_move = north(forward) &~ occ;
	bitboard double_move = north(single_move & RANK3) &~ occ;

	bitboard east_capture = north(east(normal_pawns)) & enemy;
	bitboard west_capture = north(west(normal_pawns)) & enemy;

	bitboard pinned_east_capture = north(east(pawns & info.vpinned)) & enemy & info.vpinned;
	bitboard pinned_west_capture = north(west(pawns & info.vpinned)) & enemy & info.vpinned;

	single_move  = single_move & targets;
	double_move  = double_move & targets;
	east_capture = (east_capture | pinned_east_capture) & targets;
	west_capture = (west_capture | pinned_west_capture) & targets;

	buffer->pawn_push = (single_move &~ RANK8) | double_move;

	// promotions, note: double moves cannot promote
        generate_partial_pawn_moves(buffer, single_move  & RANK8, N,   true);
        generate_partial_pawn_moves(buffer, east_capture & RANK8, N+E, true);
        generate_partial_pawn_moves(buffer, west_capture & RANK8, N+W, true);

        generate_partial_pawn_moves(buffer, east_capture &~ RANK8, N+E, false);
        generate_partial_pawn_moves(buffer, west_capture &~ RANK8, N+W, false);
}


bitboard generic_attacks(piecetype piece, square sq, bitboard occ)
{
	switch (piece) {
		case KNIGHT: return knight_attacks[sq];
		case BISHOP: return bishop_attacks(sq, occ);
		case ROOK:   return   rook_attacks(sq, occ);
		case QUEEN:  return bishop_attacks(sq, occ) | rook_attacks(sq, occ);
		default: __builtin_unreachable();
	}
}


void generate_piece_moves(movebuffer *buffer, movegen_info info, piecetype piece, board board, bool pinned)
{
	bitboard _pinned = info.hpinned | info.vpinned;
	if (pinned) _pinned = (piece == BISHOP) ? info.vpinned : info.hpinned;

	bitboard occ    = occupied(board);
	bitboard pieces = extract(board, piece);
	bitboard queens = extract(board, QUEEN);
	if (pinned) pieces |= queens;

	pieces &= board.white & (pinned ? _pinned : ~_pinned);

	for bits(pieces) {
		square init = ctz(pieces);
		bitboard attacks = generic_attacks(piece, init, occ) & info.targets;
		piecetype p = piece;

		// If the piece is pinned, then the moves must remain aligned to the king.
		if (pinned) {
			attacks &= _pinned;
			if (queens & pieces & -pieces) p = QUEEN;
		}

		for bits(attacks) {
			square dest = ctz(attacks);
			append_move(buffer, M(init, dest, p));
		}
	}
}


// Generate king moves. We use a specialised function rather than the one above as we always have
// exactly one king so the outer loop can be optimised away. Here we also clear the attacked mask
// to prevent our king from walking into check.

void generate_king_moves(movebuffer *buffer, movegen_info info, board board)
{
	bitboard occ = occupied(board);
	bitboard attacks = king_attacks[info.king] &~ (info.attacked | (board.white & occ));

	for bits(attacks) {
		square dest = ctz(attacks);
		append_move(buffer, M(info.king, dest, KING));
	}

	bitboard castling = extract(board, CASTLE) & rook_attacks(info.king, occ);

	// Special bitboards to check castling against. The OCC bitboards must not be occupied, and
	// the ATT bitboards must not be attacked, as castling out-of, through or into check is not
	// allowed.

#define QATT  (1 << C1 | 1 << D1 | 1 << E1)
#define KATT  (1 << E1 | 1 << F1 | 1 << G1)

	if (castling & (1 << A1) && !(info.attacked & QATT))  append_move(buffer, M(E1, C1, KING) | M_CASTLING);
	if (castling & (1 << H1) && !(info.attacked & KATT))  append_move(buffer, M(E1, G1, KING) | M_CASTLING);
}


// Generate attacked mask to prevent illegal king walks in parallel. We can also generate
// pawn and knight checks at the same time for efficiency.

bitboard enemy_attacked(board board, bitboard *checks)
{
	bitboard pawns   = extract(board, PAWN)   &~ board.white;
        bitboard knights = extract(board, KNIGHT) &~ board.white;
        bitboard bishops = extract(board, BISHOP) &~ board.white;
        bitboard rooks   = extract(board, ROOK)   &~ board.white;
        bitboard queens  = extract(board, QUEEN)  &~ board.white;
        bitboard king    = extract(board, KING)   &~ board.white;

	// Merge queens with other sliding pieces to reduce number of loops
        bishops |= queens;
        rooks   |= queens;

	bitboard attacked = 0;
	bitboard our_king = extract(board, KING) & board.white;
	bitboard occ = occupied(board) &~ our_king; // allows sliders to x-ray through our king

	// Simple non-sliding moves
	attacked |= south(east(pawns) | west(pawns));
	attacked |= king_attacks[ctz(king)];

	*checks |= pawns & north(east(our_king) | west(our_king));
	*checks |= knights & knight_attacks[ctz(our_king)];

	for bits(knights)  attacked |= knight_attacks[ctz(knights)];
	for bits(bishops)  attacked |= bishop_attacks(ctz(bishops), occ);
	for bits(rooks)    attacked |= rook_attacks(ctz(rooks), occ);

	return attacked;
}


// Generate a mask that contains all pinned pieces for the side to move. Note, this may include other
// random squares. We also generate sliding (bishop, rook and queen) checks here for efficiency.

void generate_pinned(board board, movegen_info *info, bitboard *checks)
{
        bitboard occ     = occupied(board);
        bitboard bishops = extract(board, BISHOP) &~ board.white;
        bitboard rooks   = extract(board, ROOK)   &~ board.white;
        bitboard queens  = extract(board, QUEEN)  &~ board.white;
	bitboard white = board.white & occ;

        bishops |= queens;
        rooks   |= queens;

	bitboard bishop_ray = bishop_attacks(info->king, occ);
	bitboard rook_ray = rook_attacks(info->king, occ);

	*checks |= bishop_ray & bishops;
	*checks |= rook_ray & rooks;

	bitboard nocc = occ & ~((bishop_ray | rook_ray) & white);

        bishops &= bishop_attacks(info->king, nocc);
        rooks   &= rook_attacks(info->king, nocc);

	for bits(bishops) info->vpinned |= line_between[info->king][ctz(bishops)];
	for bits(rooks) info->hpinned |= line_between[info->king][ctz(rooks)];
}


// Generate all legal moves for a given position. It is assumed that Board itself is a legal
// position, otherwise UB may occur (assumptions that we have a king may no longer be true).

movebuffer generate_moves(board board)
{
        movebuffer moves = {.count = 0};
	movegen_info info = {};

        info.king = ctz(extract(board, KING) & board.white);
	bitboard checks = 0;

	info.en_passant = board.white &~ occupied(board);
	info.targets = ~(occupied(board) & board.white); // cannot capture own pieces
        info.attacked = enemy_attacked(board, &checks);
        generate_pinned(board, &info, &checks);

        // If we are in check from more than one piece, then we can only move king otherwise
	// we must block the check, or capture the checking piece

	if (popcnt(checks) == 2) goto double_check;
	if (checks) info.targets &= line_between[info.king][ctz(checks)];

	// Generate moves of pinned pieces, note: pinned knights can never move
	if ((info.hpinned | info.vpinned) & board.white) {
		generate_piece_moves(&moves, info, BISHOP, board, true);
		generate_piece_moves(&moves, info, ROOK,   board, true);
	}

	// Generate regular moves for non-pinned pieces
	generate_pawn_moves (&moves, info, board);
	generate_piece_moves(&moves, info, KNIGHT, board, false);
	generate_piece_moves(&moves, info, BISHOP, board, false);
	generate_piece_moves(&moves, info, ROOK,   board, false);
	generate_piece_moves(&moves, info, QUEEN,  board, false);

double_check:
        generate_king_moves(&moves, info, board);
        return moves;
}


// Make a legal move on the board state and update it. Note: like generate_moves, this function
// also assumes that both board and move are legal.

board make_move(board board, move move)
{
	square init = M_INIT(move);
	square dest = M_DEST(move);
	piecetype piece = M_PIECE(move);

	bitboard clear = 1ull << init | 1ull << dest;

	bitboard occ = occupied(board);
	bitboard en_passant = board.white &~ occ;

	// Remove captured en-passant pawn & castling rook
	if (piece == PAWN)	clear |= south(en_passant & clear);
	if (move & M_CASTLING)	clear |= (dest < init) ? (1 << A1) : (1 << H1);

	// Clear necessary bits and set piece on dest square
	board.x     &= ~clear;
	board.y     &= ~clear;
	board.z     &= ~clear;

	set_square(&board, dest, piece);
	if (move & M_CASTLING)	set_square(&board, (dest + init) >> 1, ROOK);
	if (piece == KING)	board.x ^= extract(board, CASTLE) & RANK1; // remove castling rights

	// Flip white bitboard to black and update en-passant square
	bitboard black = occupied(board) &~ board.white;

	// Rotate bitboards to be from black's perspective
	board.x     = bswap(board.x);
	board.y     = bswap(board.y);
	board.z     = bswap(board.z);
	board.white = bswap(black);

	return board;
}

board make_pawn_push(board board, square dest)
{
	bitboard occ = occupied(board);
	bitboard black = occ &~ board.white;

	bitboard mask = 1ull << dest;
	bitboard down = south(mask);

	// case of double pawn move
	if (down &~ occ)  black |= down, down = south(down);

	mask |= down;
	board.x ^= mask;

	board.x     = bswap(board.x);
	board.y     = bswap(board.y);
	board.z     = bswap(board.z);
	board.white = bswap(black);

	return board;
}
