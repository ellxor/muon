#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef unsigned square;
typedef uint64_t bitboard;

enum { A1, B1, C1, D1, E1, F1, G1, H1 }; // useful first rank squares
enum { N = +8, S = -8, E = +1, W = -1 }; // board directions


#define AFILE  0x0101010101010101ull
#define HFILE  0x8080808080808080ull
#define RANK1  0x00000000000000ffull
#define RANK3  0x0000000000ff0000ull
#define RANK8  0xff00000000000000ull


bitboard north(bitboard bb) { return bb << 8; }
bitboard south(bitboard bb) { return bb >> 8; }
bitboard  east(bitboard bb) { return (bb &~ HFILE) << 1; }
bitboard  west(bitboard bb) { return (bb &~ AFILE) >> 1; }


//  Add definitions for useful CPU intrinsics functions for bitboard that are not available through
//  plain C code. Some example uses; ctz to iterate over bitboards, bswap to rotate them for a
//  color agnostic movegen and pext to hash occupancys to generate sliding moves (magic bitboards).

#ifndef __BMI2__
#error  BMI2 instrinsics are required for this chess engine.
#endif

#include <x86intrin.h>

#define bswap   __builtin_bswap64
#define clz      _lzcnt_u64
#define ctz      _tzcnt_u64
#define popcnt  __builtin_popcountll
#define pdep     _pdep_u64
#define pext     _pext_u64


// bit iterator: usage: `for bits(mask) { square index = ctz(mask); ... }`
#define bits(mask) (; (mask); (mask) &= (mask) - 1)
