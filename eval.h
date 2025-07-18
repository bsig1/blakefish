//
// Created by mrsig on 7/11/2025.
//

#ifndef EVAL_H
#define EVAL_H
#include "gameboard.h"
#include "eval_constants.h"

// --Pawn hash table stuff--
#define PAWN_HASH_SIZE (1<<18)   // e.g. 256K entries
#define PAWN_HASH_MASK (PAWN_HASH_SIZE - 1)


typedef struct {
	U64   key;    // pawn‐only Zobrist key
	int16_t eval; // cached pawn‐structure score
} PawnEntry;

// the table
static PawnEntry pawnTable[PAWN_HASH_SIZE];

// Zobrist keys: one random 64‐bit per side per square
static U64 pawnKeys[2][64];
// current pawn‐only key
static U64 pawnHashKey;
int get_eval(const Game *game);
static U64 passed_mask_white[64], passed_mask_black[64];
static U64 outpost_defenders[2][64], outpost_attackers[2][64];

#endif //EVAL_H
