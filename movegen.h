//
// Created by mrsig on 3/13/2025.
//

#pragma once

#ifndef MOVE_GEN_H
#define MOVE_GEN_H
#include "bitboards.h"
#include "gameboard.h"





// Returns possible moves for a piece at 'start' square
bool get_possible_moves(const Game *game, Move* move_buffer, int *move_count);

// Prints a list of moves to stdout
void print_moves(const Move* moves, int size);
void print_legal_moves(const Game *game);


#endif //MOVE_GEN_H
