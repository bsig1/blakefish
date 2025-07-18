//
// Created by mrsig on 7/11/2025.
//

#include "bitboards.h"

#include <stdio.h>
#include <stdlib.h>

#include "gameboard.h"
#include "magic.h"

U64 knight_moves [64];
U64 king_moves [64];

U64 white_pawn_pushes [64];
U64 white_pawn_double_pushes [64];
U64 white_pawn_attacks [64];

U64 black_pawn_pushes [64];
U64 black_pawn_double_pushes [64];
U64 black_pawn_attacks [64];

U64** bishop_magic_attack_table;
U64** rook_magic_attack_table;
int bishop_table_sizes[64];
int rook_table_sizes[64];

U64 rook_rays[64][64]; // First index is the start, second index is the end.
U64 bishop_rays[64][64];

U64 knight_movegen(const U64 knight) {
	U64 l1 = (knight >> 1) & ~FILE_H;
	U64 l2 = (knight >> 2) & ~FILE_GH;
	U64 r1 = (knight << 1) & ~FILE_A;
	U64 r2 = (knight << 2) & ~FILE_AB;

	return (l1 << 16) | (l1 >> 16) |
		   (l2 << 8) | (l2 >> 8) |
		   (r1 << 16) | (r1 >> 16) |
		   (r2 << 8) | (r2 >> 8);
}

U64 king_movegen(const U64 king) {
	U64 l1 = (king >> 1) & ~FILE_H;
	U64 r1 = (king << 1) & ~FILE_A;
	return (l1 | r1 | l1 << 8 | l1 >> 8 | r1 << 8 | r1 >> 8 | king << 8 | king >> 8);
}

U64 generate_pawn_pushes(const U64 pawn, const Color color) {
	// White is 1 black is -1
	if (color == White) {
		return pawn << 8;
	}
	if (color == Black) {
		return pawn >> 8;
	}
	return 0Ull;
}

U64 generate_pawn_double_pushes(const U64 pawn, const Color color) {
	if (color == White && pawn & RANK_2) {
		return pawn << 16;
	}
	if (color == Black && pawn & RANK_7) {
		return pawn >> 16;
	}
	return 0ULL;
}

U64 generate_pawn_attacks(const U64 pawn, const Color color) {
	if (color == White) {
		return ((pawn << 9) & ~FILE_A) | (pawn << 7 & ~FILE_H);
	}
	if (color == Black) {
		return ((pawn >> 9) & ~FILE_H) | ((pawn >> 7) & ~FILE_A);
	}
	return 0ULL;
}

U64 bishop_moves_onthefly(const U64 bishop, const U64 blockers) {
	U64 attacks = 0ULL;

	int square = __builtin_ctzll(bishop);
	int sq_rank = square / 8;
	int sq_file = square % 8;

	// Northeast (+9)
	for (int r = sq_rank + 1, f = sq_file + 1; r <= 7 && f <= 7; r++, f++) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
		if (current_move & blockers) {
			break;
		}
	}

	// Northwest (+7)
	for (int r = sq_rank + 1, f = sq_file - 1; r <= 7 && f >= 0; r++, f--) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
		if (current_move & blockers) {
			break;
		}
	}

	// Southeast (-7)
	for (int r = sq_rank - 1, f = sq_file + 1; r >= 0 && f <= 7; r--, f++) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
		if (current_move & blockers) {
			break;
		}
	}

	// Southwest (-9)
	for (int r = sq_rank - 1, f = sq_file - 1; r >= 0 && f >= 0; r--, f--) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
		if (current_move & blockers) {
			break;
		}
	}
	return attacks;
}

U64 rook_moves_onthefly(const U64 rook, const U64 blockers) {
	U64 attacks = 0ULL;
	int square = __builtin_ctzll(rook);
	int file = square % 8;

	// North
	for (int sq = square + 8; sq <= 63; sq += 8) {
		U64 current = 1ULL << sq;
		attacks |= current;
		if (current & blockers) break;
	}

	// South
	for (int sq = square - 8; sq >= 0; sq -= 8) {
		U64 current = 1ULL << sq;
		attacks |= current;
		if (current & blockers) break;
	}

	// East
	for (int f = file + 1, sq = square + 1; f <= 7; f++, sq++) {
		U64 current = 1ULL << sq;
		attacks |= current;
		if (current & blockers) break;
	}

	// West
	for (int f = file - 1, sq = square - 1; f >= 0; f--, sq--) {
		U64 current = 1ULL << sq;
		attacks |= current;
		if (current & blockers) break;
	}

	return attacks;
}

U64 generate_rook_mask(const int square) {
	U64 mask = 0ULL;
	int rank = square / 8;
	int file = square % 8;

	// Horizontal (left/right), skipping edges
	for (int f = file + 1; f <= 6; f++) {
		mask |= 1ULL << (rank * 8 + f);
	}
	for (int f = file - 1; f >= 1; f--) {
		mask |= 1ULL << (rank * 8 + f);
	}

	// Vertical (up/down), skipping edges
	for (int r = rank + 1; r <= 6; r++) {
		mask |= 1ULL << (r * 8 + file);
	}
	for (int r = rank - 1; r >= 1; r--) {
		mask |= 1ULL << (r * 8 + file);
	}

	return mask;
}

U64 generate_bishop_mask(const int square) {
	U64 attacks = 0ULL;

	int sq_rank = square / 8;
	int sq_file = square % 8;

	// Northeast (+9)
	for (int r = sq_rank + 1, f = sq_file + 1; r <= 6 && f <= 6; r++, f++) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
	}

	// Northwest (+7)
	for (int r = sq_rank + 1, f = sq_file - 1; r <= 6 && f >= 1; r++, f--) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
	}

	// Southeast (-7)
	for (int r = sq_rank - 1, f = sq_file + 1; r >= 1 && f <= 6; r--, f++) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
	}

	// Southwest (-9)
	for (int r = sq_rank - 1, f = sq_file - 1; r >= 1 && f >= 1; r--, f--) {
		U64 current_move = 1ULL << (r * 8 + f);
		attacks |= current_move;
	}
	return attacks;
}


void print_bitboard(const U64 piece_bitboard) {
	printf("\n  a b c d e f g h\n");
	for (int rank = 7; rank >= 0; rank--) {
		printf("%d ", rank + 1);
		for (int file = 0; file < 8; file++) {
			int sq = rank * 8 + file;
			uint64_t mask = 1ULL << sq;
			printf("%c ", (piece_bitboard & mask) ? 'x' : '.');
		}
		printf("\n");
	}
	printf("\n");
}



bool on_same_line(const int start, const int end) {
	const int start_file = start % 8, start_rank = end / 8;
	const int end_file = start % 8, end_rank = end / 8;
	return (start_file == end_file || start_rank == end_rank ||
			(start_file - start_rank) == (end_file - end_rank) ||
			(start_file + start_rank) == (end_file + end_rank));
}

U64 generate_rook_ray(int start, int end) {
	int df = (end % 8) - (start % 8);
	int dr = (end / 8) - (start / 8);

	// must share file or rank
	if (df != 0 && dr != 0)
		return 0ULL;

	int file_step = (df > 0) - (df < 0);  // +1, 0, or -1
	int rank_step = (dr > 0) - (dr < 0);

	U64 ray = 0ULL;
	int f = (start % 8) + file_step;
	int r = (start / 8) + rank_step;

	// walk until we reach 'end', excluding it
	while (f != (end % 8) || r != (end / 8)) {
		ray |= 1ULL << (r * 8 + f);
		f += file_step;
		r += rank_step;
	}

	return ray;
}

U64 generate_bishop_ray(int start, int end) {
	int df = (end % 8) - (start % 8);
	int dr = (end / 8) - (start / 8);

	// must lie on a 45° diagonal
	if (abs(df) != abs(dr))
		return 0ULL;

	int file_step = (df > 0) - (df < 0);  // +1 or -1
	int rank_step = (dr > 0) - (dr < 0);

	U64 ray = 0ULL;
	int f = (start % 8) + file_step;
	int r = (start / 8) + rank_step;

	// walk until we reach 'end', excluding it
	while (f != (end % 8) || r != (end / 8)) {
		ray |= 1ULL << (r * 8 + f);
		f += file_step;
		r += rank_step;
	}

	return ray;
}

void init_bitboards() {
	for (int rank = 0; rank < 8; rank++) {
		for (int file = 0; file < 8; file++) {
			const int square = rank * 8 + file;
			const U64 bb = 1ULL << square;

			knight_moves[square] = knight_movegen(bb);
			king_moves[square] = king_movegen(bb);

			white_pawn_pushes[square] = generate_pawn_pushes(bb, White);
			white_pawn_double_pushes[square] = generate_pawn_double_pushes(bb, White);
			white_pawn_attacks[square] = generate_pawn_attacks(bb, White);

			black_pawn_pushes[square] = generate_pawn_pushes(bb, Black);
			black_pawn_double_pushes[square] = generate_pawn_double_pushes(bb, Black);
			black_pawn_attacks[square] = generate_pawn_attacks(bb, Black);
		}
	}
	bishop_magic_attack_table = build_magic_attack_table(bishop_magics, bishop_shifts, generate_bishop_mask,
																	bishop_moves_onthefly,
																	bishop_table_sizes);
	rook_magic_attack_table = build_magic_attack_table(rook_magics, rook_shifts, generate_rook_mask,
																  rook_moves_onthefly, rook_table_sizes);


	for (int start = 0; start < 64; start++) {
		for (int end = 0; end < 64; end++) {
			bishop_rays[start][end] = generate_bishop_ray(start, end);
			rook_rays[start][end] = generate_rook_ray(start, end);
		}
	}
}

void free_magic_attack_table(U64 **table, const int table_lengths) {
	if (!table) return;

	for (int i = 0; i < table_lengths; i++) {
		free(table[i]); // free the attack table for each square
	}
	free(table); // free the outer pointer
}

void free_magic_tables() {
	if (bishop_magic_attack_table) free_magic_attack_table(bishop_magic_attack_table, 64);
	if (rook_magic_attack_table) free_magic_attack_table(rook_magic_attack_table, 64);
}

int get_magic_index(const int square, const U64 blockers, const Piece piece) {
	U64 index = 0;
	if (piece == Bishop) {
		index = (blockers * bishop_magics[square]) >> bishop_shifts[square];
	} else if (piece == Rook) {
		index = (blockers * rook_magics[square]) >> rook_shifts[square];
	}
	return (int) index;
}