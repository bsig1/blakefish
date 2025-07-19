//
// Created by mrsig on 3/13/2025.
//

#pragma once

#ifndef MOVE_GEN_H
#define MOVE_GEN_H
#include "bitboards.h"
#include "gameboard.h"


#define WHITE_KINGSIDE_OCCUPANCY_MASK     ((1ULL << 5)  | (1ULL << 6))
#define BLACK_KINGSIDE_OCCUPANCY_MASK     ((1ULL << 61) | (1ULL << 62))

#define WHITE_QUEENSIDE_OCCUPANCY_MASK    ((1ULL << 1)  | (1ULL << 2)  | (1ULL << 3))
#define BLACK_QUEENSIDE_OCCUPANCY_MASK    ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))

#define WHITE_KINGSIDE_CONTROL_MASK       ((1ULL << 4)  | (1ULL << 5)  | (1ULL << 6))
#define WHITE_QUEENSIDE_CONTROL_MASK      ((1ULL << 2)  | (1ULL << 3)  | (1ULL << 4))

#define BLACK_KINGSIDE_CONTROL_MASK       ((1ULL << 60) | (1ULL << 61) | (1ULL << 62))
#define BLACK_QUEENSIDE_CONTROL_MASK      ((1ULL << 58) | (1ULL << 59) | (1ULL << 60))


// Returns possible moves for a piece at 'start' square
Game_State get_possible_moves(const Game *game, Move* move_buffer, int *move_count);

// Prints a list of moves to stdout
void print_moves(const Move* moves, int size);
void print_legal_moves(const Game *game);


#endif //MOVE_GEN_H
