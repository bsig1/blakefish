//
// Created by mrsig on 3/13/2025.
//
#include "magic.h"
#include "movegen.h"

#include <stdio.h>
#include <stdlib.h>



static bool en_passant_is_legal(const int king_sq, const int captured_pawn_sq, const U64 occupancy, const U64 enemy_rooks,
						 const U64 enemy_queens, const U64 **rook_bitboards) {
	if ((king_sq / 8) != (captured_pawn_sq / 8)) return true;

	// Remove the captured pawn as if en passant had just occurred
	const U64 new_occupancy = occupancy & ~(1ULL << captured_pawn_sq);

	// Simulate a rook attack from the king's square
	const U64 blockers = new_occupancy & rook_masks[king_sq];
	const int magic_index = get_magic_index(king_sq, blockers, Rook);
	const U64 attack = rook_bitboards[king_sq][magic_index];

	// If any rook or queen now attacks the king, the en passant is illegal
	if (attack & (enemy_rooks | enemy_queens)) {
		return false; // illegal
	}
	return true; // legal
}

static U64 generate_white_pawn_moves(const int square,const int king_sq,const Game *game,const Gameboard *board){
    const U64 occ            = occupied_squares(board);
    const U64 enemy          = board->black_pieces;
    const U64 push_mask      = white_pawn_pushes[square];
    const U64 double_push    = white_pawn_double_pushes[square];
    const U64 attack_mask    = white_pawn_attacks[square];
    const U64 ep_mask        = game->legal_enpassant_squares;

    // Single push
    U64 blockers = occ & push_mask;
    U64 moves    = push_mask & ~blockers;

    // Double push: only if first square clear and destination clear
    blockers = (blockers << 8) | (occ & double_push);
    moves |= double_push & ~blockers;

    // Captures
    moves |= attack_mask & enemy;

    // En passant
    U64 possible_ep = attack_mask & ep_mask;
    while (possible_ep) {
        const int target = __builtin_ctzll(possible_ep);
        possible_ep &= possible_ep - 1;

        const int captured_sq = target - 8;
        const U64 enemy_rooks  = board->black_pieces & board->rooks;
        const U64 enemy_queens = board->black_pieces & board->queens;

        if (en_passant_is_legal(king_sq, captured_sq, occ,
                                enemy_rooks, enemy_queens,
                                rook_magic_attack_table)) {
            moves |= 1ULL << target;
        }
    }

    // Exclude own pieces
    return moves & ~board->white_pieces;
}


static U64 generate_black_pawn_moves(const int square,const int king_sq,const Game *game,const Gameboard *board){
    const U64 occ            = occupied_squares(board);
    const U64 enemy          = board->white_pieces;
    const U64 push_mask      = black_pawn_pushes[square];
    const U64 double_push    = black_pawn_double_pushes[square];
    const U64 attack_mask    = black_pawn_attacks[square];
    const U64 ep_mask        = game->legal_enpassant_squares;

    // Single push
    U64 blockers = occ & push_mask;
    U64 moves    = push_mask & ~blockers;

    // Double push
    blockers = (blockers >> 8) | (occ & double_push);
    moves |= double_push & ~blockers;

    // Captures
    moves |= attack_mask & enemy;

    // En passant
    U64 possible_ep = attack_mask & ep_mask;
    while (possible_ep) {
        const int target = __builtin_ctzll(possible_ep);
        possible_ep &= possible_ep - 1;

        const int captured_sq = target + 8;
        const U64 enemy_rooks  = board->white_pieces & board->rooks;
        const U64 enemy_queens = board->white_pieces & board->queens;

        if (en_passant_is_legal(king_sq, captured_sq, occ,
                                enemy_rooks, enemy_queens,
                                rook_magic_attack_table)) {
            moves |= 1ULL << target;
        }
    }

    // Exclude own pieces
    return moves & ~board->black_pieces;
}


static U64 generate_castling_moves(const int king_sq, const Color color, const U64 occupied, const U64 controlled,
							const bool castling_rights[4]) {
	U64 moves = 0;

	if (color == White && king_sq == 4) {
		// Kingside: squares f1 (5), g1 (6)
		if (castling_rights[WHITE_KINGSIDE] && !(occupied & WHITE_KINGSIDE_OCCUPANCY_MASK) && !(controlled & WHITE_KINGSIDE_CONTROL_MASK))
			moves |= 1ULL << 6;


		// Queenside: squares c1 (2), d1 (3)
		if (castling_rights[WHITE_QUEENSIDE] && !(occupied & WHITE_QUEENSIDE_OCCUPANCY_MASK) && !(controlled & WHITE_QUEENSIDE_CONTROL_MASK))
			moves |= 1ULL << 2;

	} else if (color == Black) {
		// Kingside: squares f8 (61), g8 (62)
		if (castling_rights[BLACK_KINGSIDE] && !(occupied & BLACK_KINGSIDE_OCCUPANCY_MASK) && !(controlled & BLACK_KINGSIDE_CONTROL_MASK))
			moves |= 1ULL << 62;

		// Queenside: squares c8 (58), d8 (59)
		if (castling_rights[BLACK_QUEENSIDE] && !(occupied & BLACK_QUEENSIDE_OCCUPANCY_MASK) && !(controlled & BLACK_QUEENSIDE_CONTROL_MASK))
			moves |= 1ULL << 58;
	}

	return moves;
}

Game_State get_possible_moves(const Game *game, Move* move_buffer, int *move_count) {
	*move_count = 0;
	if (game==NULL) {
		return ERROR;
	}

	const Gameboard *board = game->board;
	const U64 occupied = occupied_squares(board);

	U64 friendly_bitboard;
	U64 enemy_bitboard;

	if (game->turn == White) {
		friendly_bitboard = board->white_pieces;
		enemy_bitboard = board->black_pieces;
	} else {
		friendly_bitboard = board->black_pieces;
		enemy_bitboard = board->white_pieces;
	}

	U64 controlled_squares = 0;
	const U64 king_sq = friendly_bitboard&game->board->kings;
	const int king = __builtin_ctzll(king_sq);

	int checks[2]; // Maximum checks is 2
	U64 check_rays[2] = {0, 0};
	int check_count = 0;
	int pinning_pieces[MAX_PINS];
	U64 pin_rays[MAX_PINS];
	U64 pinned_pieces = 0;
	int pin_count = 0;

	// Find all pins, checks, and controlled squares of enemy pieces
	//const int square = pop_lsb(&enemy_bitboard);
	//const Piece piece_type = get_square_piece(number_to_square(square), board);
	U64 blockers = 0;
	U64 attacks = 0;
	U64 attacking_ray = 0;

	U64 current_pieces = 0;
	int square = 0;

	// Kings
	current_pieces = enemy_bitboard&game->board->kings;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		attacks = king_moves[square];
		controlled_squares |= attacks;
		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = 0;
			check_count++;
		}
	}

	// Knights
	current_pieces = enemy_bitboard&game->board->knights;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		attacks = knight_moves[square];
		controlled_squares |= attacks;
		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = 0;
			check_count++;
		}
	}

	// Pawns
	current_pieces = enemy_bitboard&game->board->pawns;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		if (game->turn==White) {
			attacks = black_pawn_attacks[square];
			controlled_squares |= attacks;
		}
		else {
			attacks = white_pawn_attacks[square];
			controlled_squares |= attacks;
		}

		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = 0;
			check_count++;
		}
	}

	// Bishops
	current_pieces = enemy_bitboard&game->board->bishops;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		blockers = (occupied&~king_sq) & bishop_masks[square];
		attacks = bishop_magic_attack_table[square][get_magic_index(square, blockers, Bishop)];
		controlled_squares |= attacks;
		attacking_ray = bishop_rays[king][square];

		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = attacking_ray;
			check_count++;
		} else if (attacking_ray) {
			const int blocker_count = popcount(attacking_ray & occupied);
			if (blocker_count == 1) {
				U64 blocker = attacking_ray & friendly_bitboard;
				if (blocker) {
					pin_rays[pin_count] = attacking_ray;
					pinned_pieces |= blocker;
					pinning_pieces[pin_count] = square;
					pin_count++;
				}
			}
		}
	}

	// Rooks
	current_pieces = enemy_bitboard&game->board->rooks;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		blockers = (occupied&~king_sq) & rook_masks[square];
		attacks = rook_magic_attack_table[square][get_magic_index(square, blockers, Rook)];
		controlled_squares |= attacks;
		attacking_ray = rook_rays[king][square];

		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = attacking_ray;
			check_count++;
		} else if (attacking_ray) {
			const int blocker_count = popcount(attacking_ray & occupied);
			if (blocker_count == 1) {
				U64 blocker = attacking_ray & friendly_bitboard;
				if (blocker) {
					pin_rays[pin_count] = attacking_ray;
					pinned_pieces |= blocker;
					pinning_pieces[pin_count] = square;
					pin_count++;
				}
			}
		}
	}

	// Queens
	current_pieces = enemy_bitboard&game->board->queens;
	while (current_pieces) {
		square = pop_lsb(&current_pieces);
		blockers = (occupied&~king_sq) & bishop_masks[square];
		attacks = bishop_magic_attack_table[square][get_magic_index(square, blockers, Bishop)];
		controlled_squares |= attacks;

		blockers = (occupied&~king_sq) & rook_masks[square];
		attacks |= rook_magic_attack_table[square][get_magic_index(square, blockers, Rook)];
		controlled_squares |= attacks;

		attacking_ray = bishop_rays[king][square] | rook_rays[king][square];
		if (attacks&king_sq) {
			checks[check_count] = square;
			check_rays[check_count] = attacking_ray;
			check_count++;
		} else if (attacking_ray) {
			const int blocker_count = popcount(attacking_ray & occupied);
			if (blocker_count == 1) {
				U64 blocker = attacking_ray & friendly_bitboard;
				if (blocker) {
					pin_rays[pin_count] = attacking_ray;
					pinned_pieces |= blocker;
					pinning_pieces[pin_count] = square;
					pin_count++;
				}
			}
		}
	}

	// --Calculates all legal moves of friendly pieces--
	if (check_count!=2) {
		U64 piece_on_board = 0;

		// Knights
		current_pieces = friendly_bitboard&game->board->knights;
		while (current_pieces) {
			square = pop_lsb(&current_pieces);
			attacks = knight_moves[square];

			attacks &= ~friendly_bitboard; // Cannot move to a square your color occupies
			piece_on_board = 1ULL << square;

			// If piece is pinned, it must move on that axis
			if (pinned_pieces & piece_on_board) {
				for (int pin = 0; pin < pin_count; pin++) {
					if (piece_on_board & pin_rays[pin]) {
						attacks &= pin_rays[pin] | (1ULL<<pinning_pieces[pin]);
					}
				}
			}

			while (attacks){
				const int target_sq = pop_lsb(&attacks);
				// Capture or block the check
				if (check_count) {
					if (!(target_sq == checks[0] || (1ULL << target_sq) & check_rays[0])) {
						continue;
					}
				}
				Move move = { .start = square, .end = target_sq, .promotion = Empty };
				move_buffer[(*move_count)++] = move;
			}
		}

		// Pawns
		current_pieces = friendly_bitboard&game->board->pawns;
		while (current_pieces) {
			square = pop_lsb(&current_pieces);
			if (game->turn==White) {
				attacks = generate_white_pawn_moves(square,king,game,board);
			} else {
				attacks = generate_black_pawn_moves(square,king,game,board);
			}

			attacks &= ~friendly_bitboard; // Cannot move to a square your color occupies
			piece_on_board = 1ULL << square;

			// If piece is pinned, it must move on that axis
			if (pinned_pieces & piece_on_board) {
				for (int pin = 0; pin < pin_count; pin++) {
					if (piece_on_board & pin_rays[pin]) {
						attacks &= pin_rays[pin] | (1ULL<<pinning_pieces[pin]);
					}
				}
			}

			while (attacks){
				const int target_sq = pop_lsb(&attacks);

				// Capture or block the check
				if (check_count) {
					if (!(target_sq == checks[0] || (1ULL << target_sq) & check_rays[0])) {
						continue;
					}
				}

				Move move = { .start = square, .end = target_sq, .promotion = Empty };
				if ((1ULL<<target_sq) & RANK_1_8) {
					for (int p = 0; p < PROMOTION_TYPES; p++) {
						move.promotion = (Piece)p;
						move_buffer[(*move_count)++] = move;
					}
				}
				move_buffer[(*move_count)++] = move;
			}

		}

		// Bishops
		current_pieces = friendly_bitboard&game->board->bishops;
		while (current_pieces) {
			square = pop_lsb(&current_pieces);
			blockers = occupied & bishop_masks[square];
			attacks = bishop_magic_attack_table[square][get_magic_index(square, blockers, Bishop)];
			attacks &= ~friendly_bitboard;// Cannot move to a square your color occupies
			piece_on_board = 1ULL << square;

			// If piece is pinned, it must move on that axis
			if (pinned_pieces & piece_on_board) {
				for (int pin = 0; pin < pin_count; pin++) {
					if (piece_on_board & pin_rays[pin]) {
						attacks &= pin_rays[pin] | (1ULL<<pinning_pieces[pin]);
					}
				}
			}

			while (attacks){
				const int target_sq = pop_lsb(&attacks);
				// Capture or block the check
				if (check_count) {
					if (!(target_sq == checks[0] || (1ULL << target_sq) & check_rays[0])) {
						continue;
					}
				}
				Move move = { .start = square, .end = target_sq, .promotion = Empty };
				move_buffer[(*move_count)++] = move;
			}
		}

		// Rooks
		current_pieces = friendly_bitboard&game->board->rooks;
		while (current_pieces) {
			square = pop_lsb(&current_pieces);
			blockers = occupied & rook_masks[square];
			attacks = rook_magic_attack_table[square][get_magic_index(square, blockers, Rook)];
			attacks &= ~friendly_bitboard;// Cannot move to a square your color occupies
			piece_on_board = 1ULL << square;

			// If piece is pinned, it must move on that axis
			if (pinned_pieces & piece_on_board) {
				for (int pin = 0; pin < pin_count; pin++) {
					if (piece_on_board & pin_rays[pin]) {
						attacks &= pin_rays[pin] | (1ULL<<pinning_pieces[pin]);
					}
				}
			}

			while (attacks){
				const int target_sq = pop_lsb(&attacks);
				// Capture or block the check
				if (check_count) {
					if (!(target_sq == checks[0] || (1ULL << target_sq) & check_rays[0])) {
						continue;
					}
				}
				Move move = { .start = square, .end = target_sq, .promotion = Empty };
				move_buffer[(*move_count)++] = move;
			}
		}

		// Queens
		current_pieces = friendly_bitboard&game->board->queens;
		while (current_pieces) {
			square = pop_lsb(&current_pieces);
			blockers = occupied & bishop_masks[square];
			attacks = bishop_magic_attack_table[square][get_magic_index(square, blockers, Bishop)];
			blockers = occupied & rook_masks[square];
			attacks |= rook_magic_attack_table[square][get_magic_index(square, blockers, Rook)];

			attacks &= ~friendly_bitboard;// Cannot move to a square your color occupies
			piece_on_board = 1ULL << square;
			// If piece is pinned, it must move on that axis
			if (pinned_pieces & piece_on_board) {
				for (int pin = 0; pin < pin_count; pin++) {
					if (piece_on_board & pin_rays[pin]) {
						attacks &= pin_rays[pin] | (1ULL<<pinning_pieces[pin]);
					}
				}
			}


			while (attacks){
				const int target_sq = pop_lsb(&attacks);
				// Capture or block the check
				if (check_count) {
					if (!(target_sq == checks[0] || (1ULL << target_sq) & check_rays[0])) {
						continue;
					}
				}
				Move move = { .start = square, .end = target_sq, .promotion = Empty };
				move_buffer[(*move_count)++] = move;
			}
		}
	}

	// King
	attacks = king_moves[king];

	attacks &= ~friendly_bitboard; // Cannot move to a square your color occupies
	attacks &= ~controlled_squares; // Cannot move into check

	attacks |= generate_castling_moves(king,game->turn,occupied,controlled_squares,game->castling_rights);

	while (attacks){
		const int target_sq = pop_lsb(&attacks);
		Move move = { .start = king, .end = target_sq, .promotion = Empty };
		move_buffer[(*move_count)++] = move;
	}

	if ((*move_count)==0){
		if (checks[0]) {
			return Checkmate;
		}
		else {
			return Stalemate;
		}
	}
	return Normal;
}

void print_moves(const Move *moves, const int size) {
	if (moves==NULL)return;
	for (int i = 0; i < size; i++) {
		printf("(%c %d) -> (%c %d)\n", file_to_char(moves[i].start % 8), moves[i].start / 8 + 1,
			   file_to_char(moves[i].end % 8), moves[i].end / 8 + 1);
	}
}

void print_legal_moves(const Game *game) {
	int move_count = 0;
	Move *moves = malloc(sizeof(Move) * MAX_MOVES);
	if (moves==NULL)return;
	get_possible_moves(game,moves, &move_count);
	print_moves(moves, move_count);
	printf("Total moves: %d",move_count);
	free(moves);
}