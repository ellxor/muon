#pragma once
#include <stdint.h>

/* store chess pieces in native 64-bit words (https://www.chessprogramming.org/Bitboards)
 */
typedef  uint8_t square;
typedef uint64_t bitboard;

enum { A1, B1, C1, D1, E1, F1, G1, H1 }; /* some useful first rank squares */
enum { N = +8, S = -8, E = +1, W = -1 }; /* some useful directions */

/* some useful bitboards */
constexpr bitboard AFILE = 0x0101010101010101;
constexpr bitboard HFILE = 0x8080808080808080;
constexpr bitboard RANK1 = 0x00000000000000FF;
constexpr bitboard RANK3 = 0x0000000000FF0000;
constexpr bitboard RANK8 = 0xFF00000000000000;

/* shift bitboards and truncate off edge of board */
static inline bitboard north(bitboard bb) { return bb << 8; }
static inline bitboard south(bitboard bb) { return bb >> 8; }
static inline bitboard  east(bitboard bb) { return (bb &~ HFILE) << 1; }
static inline bitboard  west(bitboard bb) { return (bb &~ AFILE) >> 1; }

/* some useful CPU intrinsics:
 *	- byteswap: reverse bitboard ranks - view from other sides perspective
 *	- popcount: count number of ones (pieces) - find multiple checkers / pinned pieces
 *	- trailing_zeros: get bit index of least significant set bit - iterating over bitboards
 */

static inline bitboard     byteswap(bitboard bb) { return __builtin_bswap64(bb);    }
static inline unsigned     popcount(bitboard bb) { return __builtin_popcountll(bb); }
static inline square trailing_zeros(bitboard bb) { return __builtin_ctzll(bb);      }

/* some bmi2 intrinsics for extra speed on newer CPUs:
 *	- pext: extract bits using mask to create an index for lookup tables
 */

#ifndef USE_PEXT
#error "Non-bmi2 platforms are not supported yet"
#endif

#include <x86intrin.h>
static inline bitboard pext(bitboard bb, bitboard mask) { return _pext_u64(bb, mask); }

