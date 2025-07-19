//
// Created by mrsig on 7/11/2025.
//

#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "gameboard.h"





extern U64 knight_moves [64];
extern U64 king_moves [64];

extern U64 white_pawn_pushes [64];
extern U64 white_pawn_double_pushes [64];
extern U64 white_pawn_attacks [64];

extern U64 black_pawn_pushes [64];
extern U64 black_pawn_double_pushes [64];
extern U64 black_pawn_attacks [64];

extern U64** bishop_magic_attack_table;
extern U64** rook_magic_attack_table;
extern int bishop_table_sizes[64];
extern int rook_table_sizes[64];

extern U64 rook_rays[64][64]; // First index is the start, second index is the end.
extern U64 bishop_rays[64][64];




// Move generation
int popcount(U64 x);
U64 knight_movegen(U64 knight);
U64 king_movegen(U64 king);
U64 generate_pawn_pushes(U64 pawn, Color color);
U64 generate_pawn_double_pushes(U64 pawn, Color color);
U64 generate_pawn_attacks(U64 pawn, Color color);
U64 bishop_moves_onthefly(U64 bishop, U64 blockers);
U64 rook_moves_onthefly(U64 rook, U64 blockers);

// Mask generation
U64 generate_rook_mask(int square);
U64 generate_bishop_mask(int square);



// Bitboard utility
void print_bitboard(U64 piece_bitboard);

// Piece_Bitboards init & cleanup
void init_bitboards();
void free_magic_tables();

// Lookup
int get_magic_index(int square, U64 blockers,Piece piece);
#endif //BITBOARDS_H
