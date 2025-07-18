//
// Created by mrsig on 3/13/2025.
//

#pragma once

#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SQUARES 64
#define MAX_PINS 8 // there can never be more than 8 pins, 4 sides 4 diagonals
#define MAX_MOVES 218 // 218 is the max possible moves in a position
#define FILE_A 0x0101010101010101ULL
#define FILE_AB 0x0303030303030303ULL
#define FILE_H 0x8080808080808080ULL
#define FILE_GH 0xC0C0C0C0C0C0C0C0ULL
#define RANK_2 0x000000000000FF00ULL
#define RANK_7 0x00FF000000000000ULL
#define RANK_1_8 0xFF000000000000FFULL
#define PROMOTION_TYPES 4


typedef uint64_t U64;

static const U64 FILE_MASKS[8] = {
	0x0101010101010101ULL, // File A
	0x0202020202020202ULL, // File B
	0x0404040404040404ULL, // File C
	0x0808080808080808ULL, // File D
	0x1010101010101010ULL, // File E
	0x2020202020202020ULL, // File F
	0x4040404040404040ULL, // File G
	0x8080808080808080ULL  // File H
};




typedef struct {
	// Board is bit addressed, file then row:
	// So all a values, then all b values etc.
	U64 black_pieces;
	U64 white_pieces;
	U64 rooks;
	U64 queens;
	U64 knights;
	U64 bishops;
	U64 kings;
	U64 pawns;
} Gameboard;



typedef enum {
	White,
	Black,
	None,
} Color;

typedef enum {
	Knight,
	Bishop,
	Queen,
	Rook,
	King,
	Pawn,
	Empty
} Piece;

typedef enum {
	WHITE_KINGSIDE = 0,
	WHITE_QUEENSIDE = 1,
	BLACK_KINGSIDE = 2,
	BLACK_QUEENSIDE = 3
} CastlingRight;

typedef enum {
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H
} File;

typedef struct {
	File file;
	int rank;
} Square;

typedef struct {
	int start;
	int end;
	Piece promotion;
} Move;

typedef struct {
	Gameboard *board;
	U64 legal_enpassant_squares;
	Color turn;
	int halfmove_clock;
	int fullmove_number;
	// White king side, queen side, then black king side, queen side
	bool castling_rights[4];
} Game;

// Initializes all bitboards and sets returns a pointer to the heap
Gameboard *init_gameboard();


// --Utility functions--
char file_to_char(File file);

File char_to_file(char c);

Piece char_to_piece(char c);

char piece_to_char(Piece piece);

// Converts a Square struct to 0–63 square number
int square_to_number(const Square *square);

// Converts a 0–63 number to a Square struct
Square number_to_square(int number);

int *get_marked_squares(U64 bitboard, int *size);

int pop_lsb(U64 *bb);

int popcount(U64 x);

U64 occupied_squares(const Gameboard *board);

// --Game Board Manipulation--
bool set_square(Square square, Color color, Piece piece, Gameboard *board);

void clear_square(int square, Gameboard* board);

Piece get_square_piece(Square square, const Gameboard *board);

void set_standard_board(Gameboard *board);

void print_board(const Gameboard *board);


#endif //BITBOARD_H
