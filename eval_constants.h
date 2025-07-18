//
// Created by blake on 7/17/25.
//



#ifndef EVAL_CONSTANTS_H
#define EVAL_CONSTANTS_H

#include "gameboard.h"




// --Eval Constants--
#define ISO_PEN       15   // Isolated pawn
#define DOUBLED_PEN   10   // Each extra pawn on a file
#define BACKWARD_PEN  10   // A pawn that can’t be safely advanced
#define ISLAND_PEN    12   // Each extra pawn‐island beyond one
#define CHAIN_BONUS    6   // Each pawn defensive “link” in a chain

// piece‐values in centi-pawns
#define PAWN_VALUE     100
#define KNIGHT_VALUE   320
#define BISHOP_VALUE   330
#define ROOK_VALUE     500
#define QUEEN_VALUE    900
#define KING_VALUE  100000   // arbitrary large value

// bonus for having the two bishops
#define BISHOP_PAIR_BONUS 20

// bonus for whoever's turn it currently is
#define TURN_BONUS 5

#define SEMI_OPEN_FILE_BONUS 10
#define OPEN_FILE_BONUS 10

#define ENDGAME_MATERIAL_LINE 70


extern int piece_square_table_middlegame[2][6][64];
extern int piece_square_table_endgame[2][6][64];
extern int passed_bonus[2][8];
extern U64 knight_outpost_defenders[2][64];
extern U64 knight_outpost_attackers[2][64];

#endif //EVAL_CONSTANTS_H
