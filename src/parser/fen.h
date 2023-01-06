#pragma once
#include <common/board.h>

Board parse_fen(const char *, bool *white_to_move, bool *ok);

