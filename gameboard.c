#include "gameboard.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BOARD_SIZE 64

Gameboard *init_gameboard() {
	Gameboard *gameboard = malloc(sizeof(U64)*10);
	gameboard->kings = 0ULL;
	gameboard->bishops = 0ULL;
	gameboard->knights = 0ULL;
	gameboard->rooks = 0;
	gameboard->queens = 0;
	gameboard->black_pieces = 0;
	gameboard->white_pieces = 0;
	gameboard->pawns = 0;
	return gameboard;
}

char file_to_char(const File file) {
	return (char)('a' + file);
}

File char_to_file(const char c) {
	switch (c) {
		case 'A': case 'a': return A;
		case 'B': case 'b': return B;
		case 'C': case 'c': return C;
		case 'D': case 'd': return D;
		case 'E': case 'e': return E;
		case 'F': case 'f': return F;
		case 'G': case 'g': return G;
		case 'H': case 'h': return H;
		default:            return (File)-1;  // invalid file
	}
}

Piece char_to_piece(const char c) {
	switch (c) {
		case 'P': return Pawn;
		case 'N': return Knight;
		case 'B': return Bishop;
		case 'R': return Rook;
		case 'Q': return Queen;
		case 'K': return King;

		case 'p': return Pawn;
		case 'n': return Knight;
		case 'b': return Bishop;
		case 'r': return Rook;
		case 'q': return Queen;
		case 'k': return King;

		default:  return Empty;
	}
}

char piece_to_char(const Piece piece) {
	switch (piece) {
		case Pawn:   return 'P';
		case Knight: return 'N';
		case Bishop: return 'B';
		case Rook:   return 'R';
		case Queen:  return 'Q';
		case King:   return 'K';
		default:     return '.';  // for None
	}
}

int square_to_number(const Square *square) {
	return (int)((square->rank - 1) * 8 + square->file);
}

Square number_to_square(const int number) {
	Square square;
	square.file = number % 8;
	square.rank = number / 8 + 1;
	return square;
}

int *get_marked_squares(const U64 bitboard, int *size) {
	int *squares = malloc(64 * sizeof(int));
	*size = 0;

	U64 b = bitboard;
	while (b) {
		const int sq = __builtin_ctzll(b); // index of LSB
		squares[(*size)++] = sq;
		b &= b - 1; // clear LSB
	}
	return squares;
}

int pop_lsb(U64 *bb) {
	// Extract index of least significant 1-bit,
	// then clear it and return its index
	const int idx = __builtin_ctzll(*bb);
	*bb &= *bb - 1;
	return idx;
}

int popcount(const U64 x) {
	return __builtin_popcountll(x); // Fast, hardware-accelerated
}

U64 occupied_squares(const Gameboard *board) {
	return board->white_pieces | board->black_pieces;
}

bool get_board_number_state(const U64 mask, const U64 *board_number) {
	return (bool) (mask & (*board_number));
}

static U64 get_mask(const Square square) {
	if (square.file < A || square.file > H || square.rank < 1 || square.rank > 8) return 0;
	return 1ULL << (square.file + (square.rank - 1) * 8);
}

bool is_square_occupied(const Square square,const Gameboard *board) {
	return (bool) (get_mask(square) & occupied_squares(board));
}

static void set_board_number(const U64 mask, U64 *number) {
	*number |= mask;
}

void clear_square(const int square, Gameboard* board) {
	const U64 mask = ~(1ULL << square);

	// Clear color gameboard

	// Clear all piece type gameboards
	board->pawns   &= mask;
	board->knights &= mask;
	board->bishops &= mask;
	board->rooks   &= mask;
	board->queens  &= mask;
	board->kings   &= mask;
	board->black_pieces &= mask;
	board->white_pieces &= mask;
}

bool set_square(const Square square,const Color color,const Piece piece, Gameboard *board) {
	const U64 mask = get_mask(square);

	switch (piece) {
		case Pawn:
			set_board_number(mask, &board->pawns);
			break;
		case Queen:
			set_board_number(mask, &board->queens);
			break;
		case King:
			set_board_number(mask, &board->kings);
			break;
		case Bishop:
			set_board_number(mask, &board->bishops);
			break;
		case Knight:
			set_board_number(mask, &board->knights);
			break;
		case Rook:
			set_board_number(mask, &board->rooks);
			break;
		default:
			return false;
	}

	if (color == White) set_board_number(mask, &board->white_pieces);
	else set_board_number(mask, &board->black_pieces);

	return true;
}


Piece get_square_piece(const Square square, const Gameboard *board) {
	const U64 mask = get_mask(square);
	if (mask & board->pawns) return Pawn;
	if (mask & board->queens) return Queen;
	if (mask & board->kings) return King;
	if (mask & board->bishops) return Bishop;
	if (mask & board->knights) return Knight;
	if (mask & board->rooks) return Rook;
	return Empty;
}

static Color get_square_color(const Square square,const Gameboard *board) {
	const U64 mask = get_mask(square);
	if (board->black_pieces & mask) return Black;
	if (board->white_pieces & mask) return White;
	return None;
}

void set_standard_board(Gameboard *board) {
	set_square((Square){A, 1}, White, Rook, board);
	set_square((Square){B, 1}, White, Knight, board);
	set_square((Square){C, 1}, White, Bishop, board);
	set_square((Square){D, 1}, White, Queen, board);
	set_square((Square){E, 1}, White, King, board);
	set_square((Square){F, 1}, White, Bishop, board);
	set_square((Square){G, 1}, White, Knight, board);
	set_square((Square){H, 1}, White, Rook, board);

	set_square((Square){A, 8}, Black, Rook, board);
	set_square((Square){B, 8}, Black, Knight, board);
	set_square((Square){C, 8}, Black, Bishop, board);
	set_square((Square){D, 8}, Black, Queen, board);
	set_square((Square){E, 8}, Black, King, board);
	set_square((Square){F, 8}, Black, Bishop, board);
	set_square((Square){G, 8}, Black, Knight, board);
	set_square((Square){H, 8}, Black, Rook, board);

	for (int i = 0; i < 8; i++) {
		set_square((Square){i, 2}, White, Pawn, board);
		set_square((Square){i, 7}, Black, Pawn, board);
	}
}

void print_board(const Gameboard *board) {
	for (int row = 8; row > 0; row--) {
		for (int file_number = 0; file_number < 8; file_number++) {
			const Square square = (Square){file_number,row};
			if (is_square_occupied(square, board)) {
				const char color_letter = get_square_color(square, board) == White ? 'W' : 'B';
				printf("%c ", color_letter);
			} else {
				printf("0 ");
			}
		}
		printf("\n");
	}
	printf("\n");

	for (int row = 8; row > 0; row--) {
		for (int file_number = 0; file_number < 8; file_number++) {
			const Square square = (Square){file_number,row};
			printf("%c ", piece_to_char(get_square_piece(square, board)));
		}
		printf("\n");
	}
	printf("\n");
}

