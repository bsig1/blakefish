//
// Created by mrsig on 7/5/2025.
//
#pragma once

#include "gameboard.h"
#include "movegen.h"

#ifndef GAME_H
#define GAME_H





typedef struct Game_Tree_Node{
	Move move;
	struct Game_Tree_Node** children;
	int child_count;
	int count;
	int eval;
} Game_Tree_Node;


typedef struct {
	U64 white_pieces, black_pieces;
	U64 pawns, knights, bishops, rooks, queens, kings;
	Color turn;
	bool castling_rights[4];
	U64 legal_enpassant_squares;
	int fullmove_number;
	int halfmove_clock;
} Game_State_Snapshot;

Game* init_game();
Game* init_startpos_game();

void make_move(const Move* move, Game *game);
Game_Tree_Node* generate_game_tree(Game* game, int max_depth, int* total_count);
void print_tree(const Game_Tree_Node *node, int depth);
U64 perft(Game* game, int depth);
void perft_divide(Game* game, int depth);
void make_move_str(const char* str, Game* game);
void make_moves_str(const char* str, Game* game);
bool set_board_from_fen(Game *game, const char *fen);
void free_game(Game* game);

#endif //GAME_H
