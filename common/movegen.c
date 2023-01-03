#include "movegen.h"

/* Generate pawn moves from a move mask, from a given direction. This allows us to
 * generate in more predictable loops.
 */
static inline
void generate_partial_pawn_moves(bitboard moves, square direction, bool promotion,
                                 MoveBuffer *buffer)
{
	while (moves) {
		square dest = trailing_zeros(moves);
		square init = dest - direction;

		if (promotion) {
			append_move(buffer, (Move) { init, dest, KNIGHT, false });
			append_move(buffer, (Move) { init, dest, BISHOP, false });
			append_move(buffer, (Move) { init, dest, ROOK,   false });
			append_move(buffer, (Move) { init, dest, QUEEN,  false });
		}

		else {
			append_move(buffer, (Move) { init, dest, PAWN, false });
		}

		moves &= moves - 1;
	}
}


/* Generate pawn moves according to a targets mask (where pawns must end their move, for example
 * in case of check this may be restricted), and a pinned mask which indicates pawns that are
 * pinned to our king.
 */
static inline
void generate_pawn_moves(Board board, bitboard targets, bitboard pinned, square king,
		         MoveBuffer *buffer)
{
	bitboard pawns = extract(board, PAWN) & board.white;
	bitboard occ   = occupied(board);
	bitboard enemy = occ &~ board.white;
	bitboard en_passant = board.white &~ occ;

	/* Check for pinned en-passant. Note that this is a special type of pinned piece as two
	 * pieces dissappear in the checking direction. This introduces a slow branch into our pawn
	 * move generation, but it is a necessary evil for full legality, however rare.
	 */
	bitboard candidates = south(east(en_passant) | west(en_passant));

	// We optimise this branch by only checking if the king is actually on the 5th rank
	if ((king & 56) == 32 && popcount(candidates) == 1)
	{
		bitboard rooks  = extract(board, ROOK)  &~ board.white;
		bitboard queens = extract(board, QUEEN) &~ board.white;

		candidates |= south(en_passant); // merge capturer and captured pawn
		rooks      |= queens;		 // merge pinning pieces
		
		/* If the pawn is "double" pinned, then en-passant is no longer possible
		 */
		if (rook_moves(king, (occ | en_passant) &~ candidates) & rooks)
			en_passant = 0;
	}

	/* In this case we enable en-passant if the pawn being capture was giving check
	 */
	targets |= en_passant & north(targets);
	enemy   |= en_passant;

	bitboard normal_pawns = pawns &~ pinned;
	bitboard pinned_pawns = pawns & pinned;

	/* Generate simple pawn moves */
	bitboard single_move = north(normal_pawns) &~ occ;
	bitboard double_move = north(single_move & RANK3) &~ occ;

	/* Handle pinned pieces: must be in line with king if pinned */
	bitboard king_file = AFILE << (king & 7);

	bitboard pinned_single_move = north(pinned_pawns) & king_file &~ occ;
	bitboard pinned_double_move = north(pinned_single_move & RANK3) &~ occ;

	/* Generate simple pawn captures */
	bitboard east_capture = north(east(normal_pawns)) & enemy;
	bitboard west_capture = north(west(normal_pawns)) & enemy;

	/* Handle pinned captures: must align diagonally with king */
	pinned_pawns &= ~rook_moves(king, 0); // orthogonal pinned pawns can never capture

	bitboard pinned_east_capture = north(east(pinned_pawns)) & enemy;
	bitboard pinned_west_capture = north(west(pinned_pawns)) & enemy;

	pinned_east_capture &= bishop_moves(king, 0);
	pinned_west_capture &= bishop_moves(king, 0);

	/* Merge bitboards and union with targets */
	single_move  = (single_move  | pinned_single_move)  & targets;
	double_move  = (double_move  | pinned_double_move)  & targets;
	east_capture = (east_capture | pinned_east_capture) & targets;
	west_capture = (west_capture | pinned_west_capture) & targets;

	/* promotions: note double move can never promote */
        generate_partial_pawn_moves(single_move  & RANK8, N,   true, buffer);
        generate_partial_pawn_moves(east_capture & RANK8, N+E, true, buffer);
        generate_partial_pawn_moves(west_capture & RANK8, N+W, true, buffer);

        /* non-promotions */
        generate_partial_pawn_moves(single_move  &~ RANK8, N,   false, buffer);
        generate_partial_pawn_moves(double_move,           N+N, false, buffer);
        generate_partial_pawn_moves(east_capture &~ RANK8, N+E, false, buffer);
        generate_partial_pawn_moves(west_capture &~ RANK8, N+W, false, buffer);
}


static inline
bitboard generic_attacks(piecetype piece, square sq, bitboard occ)
{
	switch (piece) {
		case KNIGHT: return knight_moves(sq);
		case BISHOP: return bishop_moves(sq, occ);
		case ROOK:   return   rook_moves(sq, occ);
		case QUEEN:  return  queen_moves(sq, occ);
		default: __builtin_unreachable();
	}
}


static inline
void generate_piece_moves(piecetype piece, Board board, bitboard targets, bitboard filter,
                          bool pinned, square king, MoveBuffer *buffer)
{
	bitboard pieces = extract(board, piece) & board.white & filter;
	bitboard occ    = occupied(board);

	while (pieces) {
		square init = trailing_zeros(pieces);
		bitboard attacks = generic_attacks(piece, init, occ) & targets;

		/* If we are generating pinned moves, then the destination square must be aligned
		 * to the king.
		 */
		if (pinned)
			attacks &= get_line_connecting(king, init);

		while (attacks) {
			square dest = trailing_zeros(attacks);
			append_move(buffer, (Move) { init, dest, piece, false });

			attacks &= attacks - 1;
		}

		pieces &= pieces - 1;
	}
}

/* Generate king moves. We use a specialised function rather than the one above as we always have
 * exactly one king so the outer loop can be optimised away. Here we also clear the attacked mask
 * to prevent our king from walking into check.
 */
static inline
void generate_king_moves(Board board, bitboard attacked, square king, MoveBuffer *buffer)
{
	bitboard occ = occupied(board);
	bitboard attacks = king_moves(king) &~ attacked &~ (board.white & occ);

	while (attacks) {
		square dest = trailing_zeros(attacks);
		append_move(buffer, (Move) { king, dest, KING, false });
		attacks &= attacks - 1;
	}

	/* Extact castling rights: these are all the 'castles' in the position
	 */
	bitboard castling = extract(board, CASTLE);

	/* Special bitboards to check castling against. The OCC bitboards must not be occupied, and
	 * the ATT bitboards must not be attacked, as castling out-of, through or into check is not
	 * allowed.
	 */
	constexpr bitboard KATT = (1 << E1) + (1 << F1) + (1 << G1);
	constexpr bitboard KOCC = (1 << F1) + (1 << G1);
	constexpr bitboard QATT = (1 << C1) + (1 << D1) + (1 << E1);
	constexpr bitboard QOCC = (1 << B1) + (1 << C1) + (1 << D1);

	constexpr Move  kingside = { E1, G1, KING, true };
	constexpr Move queenside = { E1, C1, KING, true };

	if (castling & (1 << A1) && !(occ & QOCC) && !(attacked & QATT))
		append_move(buffer, queenside);

	if (castling & (1 << H1) && !(occ & KOCC) && !(attacked & KATT))
		append_move(buffer, kingside);
}

/* Generate attacked mask to prevent illegal king walks in parallel. We can also generate checks
 * at the same time for efficiency.
 */
static inline
bitboard enemy_attacked(Board board, bitboard *checks)
{
	bitboard pawns   = extract(board, PAWN)   &~ board.white;
        bitboard knights = extract(board, KNIGHT) &~ board.white;
        bitboard bishops = extract(board, BISHOP) &~ board.white;
        bitboard rooks   = extract(board, ROOK)   &~ board.white;
        bitboard queens  = extract(board, QUEEN)  &~ board.white;
        bitboard king    = extract(board, KING)   &~ board.white;

	/* Merge queens with other sliding pieces to reduce number of loops */
        bishops |= queens;
        rooks   |= queens;

	bitboard attacked = 0;
	bitboard our_king = extract(board, KING) & board.white;

	/* Extract occupied squares but remove our king from this mask to allow enemy checking rays
	 * to pass through our king. This prevents us from walking backwards into check.
	 */
	bitboard occ = occupied(board) &~ our_king;

	/* Simple non-sliding moves */
	attacked |= south(east(pawns) | west(pawns));
	attacked |= king_moves(trailing_zeros(king));

	*checks |= pawns & north(east(our_king) | west(our_king));
	*checks |= knights & knight_moves(trailing_zeros(our_king));

	while (knights) {
		attacked |= knight_moves(trailing_zeros(knights));
		knights  &= knights - 1;
	}

	/* Sliding piece attacks: if our king is included in the attack mask, then we append the
	 * checking pieces to the checks mask
	 */
	while (bishops) {
		bitboard attacks = bishop_moves(trailing_zeros(bishops), occ);
		attacked |= attacks;

		*checks |= (attacks & our_king) ? (bishops &- bishops) : 0;
		bishops &= bishops - 1;
	}

	while (rooks) {
		bitboard attacks = rook_moves(trailing_zeros(rooks), occ);
		attacked |= attacks;

		*checks |= (attacks & our_king) ? (rooks &- rooks) : 0;
		rooks &= rooks - 1;
	}

	return attacked;
}


/* Generate pinned pieces ahead of time to prevent having to slowly filter moves by legality.
 */
static inline
bitboard generate_pinned(Board board, square king)
{
        bitboard occ     = occupied(board);
        bitboard bishops = extract(board, BISHOP) &~ board.white;
        bitboard rooks   = extract(board, ROOK)   &~ board.white;
        bitboard queens  = extract(board, QUEEN)  &~ board.white;

        bishops |= queens;
        rooks   |= queens;

        bishops &= bishop_moves(king, bishops);
        rooks   &= rook_moves(king, rooks);

        bitboard pinned = 0;
        bitboard candidates = bishops | rooks;

        while (candidates) {
                bitboard line = get_line_between(king, trailing_zeros(candidates)) & occ;

		/* If we only have one piece between our king and an enemy slider, then it is
		 * pinned. We don't care if the piece is friendly or not as we will only try to
		 * move our own pieces.
		 */
                if (popcount(line) == 1)
                	pinned |= line;

                candidates &= candidates - 1;
        }

        return pinned;
}


/* Generate all legal moves for a given position. It is assumed that Board itself is a legal
 * position, otherwise UB may occur (assumptions that we have a king may no longer be true).
 */
MoveBuffer generate_moves(Board board)
{
        MoveBuffer moves = {.count = 0};

        square king = trailing_zeros(extract(board, KING) & board.white);

        bitboard checks = 0;
        bitboard attacked = enemy_attacked(board, &checks);
        bitboard pinned   = generate_pinned(board, king);

        /* We cannot capture our own pieces, so we clear these in the target mask */
        bitboard targets  = ~(occupied(board) & board.white);

        /* If we are in check from more than one piece, then we can only move king otherwise we must
         * block the check, or capture the checking piece
         */
        if (checks)
                targets &= (popcount(checks) == 1)
                	 * (checks | get_line_between(king, trailing_zeros(checks)));

        /* Generate moves of pinned pieces, note: pinned knights can never move
         */
        generate_piece_moves(BISHOP, board, targets, pinned, true, king, &moves);
        generate_piece_moves(ROOK,   board, targets, pinned, true, king, &moves);
        generate_piece_moves(QUEEN,  board, targets, pinned, true, king, &moves);

	/* Generate regular moves for non-pinned pieces
	 */
        generate_pawn_moves(board, targets, pinned, king, &moves);
        generate_piece_moves(KNIGHT, board, targets, ~pinned, false, king, &moves);
        generate_piece_moves(BISHOP, board, targets, ~pinned, false, king, &moves);
        generate_piece_moves(ROOK,   board, targets, ~pinned, false, king, &moves);
        generate_piece_moves(QUEEN,  board, targets, ~pinned, false, king, &moves);
        generate_king_moves(board, attacked, king, &moves);

        return moves;
}


/* Place a piece on a given square of the board. Note: this implementation assumes the square is
 * empty, so must be cleared if previously occupuied.
 */
static inline
void set_square(Board *board, square sq, piecetype piece)
{
	board->x |= (((bitboard)piece >> 0) & 1) << sq;
        board->y |= (((bitboard)piece >> 1) & 1) << sq;
        board->z |= (((bitboard)piece >> 2) & 1) << sq;
        board->white |= 1ull << sq;
}

/* Make a legal move on the board state and update it. Note: like generate_moves, this function
 * also assumes that both board and move are legal.
 */
void make_move(Board *board, Move move)
{
	bitboard clear = (1ull << move.init) 
	               | (1ull << move.dest);

	bitboard occ = occupied(*board);
	bitboard en_passant = board->white &~ occ;

	// In case of en-passant, we clear the square (capture the pawn) below
	if (move.piece == PAWN)
		clear |= south(en_passant & clear);

	// In case of castling we remove the rook as well
	if (move.castling)
		clear |= (move.dest < move.init) ? (1 << A1) : (1 << H1);

	// Removed pieces from cleared squares
	board->x &= ~clear;
	board->y &= ~clear;
	board->z &= ~clear;
	board->white &= ~clear;

	// Move the piece itself, note: this also is correct for promotion case
	set_square(board, move.dest, move.piece);

	// If we castle, we set the middle (average) square to a rook
	if (move.castling)
		set_square(board, (move.dest + move.init) >> 1, ROOK);

	// Remove all castling rights if our king moves: toggle x bit to convert CASTLE to ROOK
	if (move.piece == KING)
		board->x ^= extract(*board, CASTLE) & RANK1;


	bitboard black = occupied(*board) &~ board->white;

	// Update en-passant square if we double pushed a pawn
	if (move.piece == PAWN && (move.dest - move.init) == N+N)
		black |= 256ull << move.init;

	// Rotate bitboards to be from black's perspective
	board->x = byteswap(board->x);
	board->y = byteswap(board->y);
	board->z = byteswap(board->z);
	board->white = byteswap(black);
}

