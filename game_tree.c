//
// Created by mrsig on 7/5/2025.
//

#include "game_tree.h"
#include "eval.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game* init_game() {
	Game *game = malloc(sizeof(Game));
	game->turn = White;
	game->fullmove_number = 0;
	game->halfmove_clock = 0;
	game->legal_enpassant_squares = 0;
	for (int i = 0; i < 4; i++) {
		game->castling_rights[i] = true;
	}
	game->board = init_gameboard();
	return game;
}

void make_move(const Move* move, Game *game) {
	if (move==NULL||game==NULL)return;

	Gameboard *board = game->board;
	const int start = move->start;
	const int end = move->end;
	const Color turn = game->turn;

	Square start_sq = number_to_square(start);
	Square end_sq = number_to_square(end);

	const Piece moving_piece = get_square_piece(start_sq, board);
	const Piece captured_piece = get_square_piece(end_sq, board);

	bool is_en_passant = false;
	if (moving_piece == Pawn && (1ULL << end) & game->legal_enpassant_squares) {
		is_en_passant = true;

		int captured_pawn_sq = (turn == White) ? end - 8 : end + 8;
		clear_square(captured_pawn_sq, board);
	}

	// Update en passant target
	if (moving_piece == Pawn && abs(end - start) == 16) {
		int ep_sq = (start + end) / 2;
		game->legal_enpassant_squares = 1ULL << ep_sq;
	} else {
		game->legal_enpassant_squares = 0ULL;
	}

	// Handle castling
	if (moving_piece == King && abs(end - start) == 2) {
		int rook_start, rook_end;
		if (end > start) {
			// Kingside castle
			rook_start = (turn == White) ? 7  : 63;  // h1 : h8
			rook_end   = end - 1;                   // f1 (5) or f8 (61)
		} else {
			// Queenside castle
			rook_start = (turn == White) ? 0  : 56;  // a1 : a8
			rook_end   = end + 1;                   // d1 (3) or d8 (59)
		}
		// Remove the rook from its original square...
		clear_square(rook_start, board);
		// ...and place it next to the king’s new square.
		set_square(number_to_square(rook_end), turn, Rook, board);

		//once you castle, you must also clear both castling rights
		if (turn == White) {
			game->castling_rights[0] = false; // White kingside
			game->castling_rights[1] = false; // White queenside
		} else {
			game->castling_rights[2] = false; // Black kingside
			game->castling_rights[3] = false; // Black queenside
		}
	}

	// Clear origin and destination
	clear_square(start, board);
	if (captured_piece != Empty && !is_en_passant)
		clear_square(end, board);

	// Handle promotion
	Piece final_piece = moving_piece;
	if (moving_piece == Pawn && move->promotion != Empty)
		final_piece = move->promotion;

	set_square(end_sq, turn, final_piece, board);

	// Handle castling rights
	if (moving_piece == King) {
		game->castling_rights[turn*2] = false;
		game->castling_rights[turn*2+1] = false;
	} else if (moving_piece == Rook) {
		if (start == 0) game->castling_rights[WHITE_KINGSIDE] = false;
		if (start == 7) game->castling_rights[WHITE_QUEENSIDE] = false;
		if (start == 56) game->castling_rights[BLACK_KINGSIDE] = false;
		if (start == 63) game->castling_rights[BLACK_QUEENSIDE] = false;
	}

	if (captured_piece == Rook && !is_en_passant) {
		if (end == 0) game->castling_rights[WHITE_KINGSIDE] = false;
		if (end == 7) game->castling_rights[WHITE_QUEENSIDE] = false;
		if (end == 56) game->castling_rights[BLACK_KINGSIDE] = false;
		if (end == 63) game->castling_rights[BLACK_QUEENSIDE] = false;
	}

	game->turn ^= 1;
	game->fullmove_number++;
	game->halfmove_clock = game->fullmove_number/2;
}

void save_game_state(const Game* game, Game_State_Snapshot* snapshot) {
	const Gameboard* b = game->board;
	snapshot->white_pieces = b->white_pieces;
	snapshot->black_pieces = b->black_pieces;
	snapshot->pawns = b->pawns;
	snapshot->knights = b->knights;
	snapshot->bishops = b->bishops;
	snapshot->rooks = b->rooks;
	snapshot->queens = b->queens;
	snapshot->kings = b->kings;
	snapshot->turn = game->turn;
	snapshot->legal_enpassant_squares = game->legal_enpassant_squares;
	snapshot->fullmove_number = game->fullmove_number;
	snapshot->halfmove_clock = game->halfmove_clock;
	for (int i = 0; i < 4; i++) snapshot->castling_rights[i] = game->castling_rights[i];
}

void load_game_state(Game* game, const Game_State_Snapshot* snapshot) {
	Gameboard* b = game->board;
	b->white_pieces = snapshot->white_pieces;
	b->black_pieces = snapshot->black_pieces;
	b->pawns = snapshot->pawns;
	b->knights = snapshot->knights;
	b->bishops = snapshot->bishops;
	b->rooks = snapshot->rooks;
	b->queens = snapshot->queens;
	b->kings = snapshot->kings;
	game->turn = snapshot->turn;
	game->fullmove_number = snapshot->fullmove_number;
	game->halfmove_clock = snapshot->halfmove_clock;
	game->legal_enpassant_squares = snapshot->legal_enpassant_squares;
	for (int i = 0; i < 4; i++) game->castling_rights[i] = snapshot->castling_rights[i];
}

char* move_to_uci(const Move* move) {
	char* move_uci = malloc(7 * sizeof(char));

	Square start_square = number_to_square(move->start);
	Square end_square = number_to_square(move->end);

	move_uci[0] = file_to_char(start_square.file);
	move_uci[1] = (char)('0'+start_square.rank);
	move_uci[2] = file_to_char(end_square.file);
	move_uci[3] = (char)('0'+end_square.rank);

	move_uci[4] = '\0';
	if (move->promotion != Empty) {
		move_uci[4] = '=';
		move_uci[5] = piece_to_char(move->promotion);
		move_uci[6] = '\0';
	}
	return move_uci;
}

Game* copy_game(const Game* original) {
	Game* copy = malloc(sizeof(Game));
	if (!copy) return NULL;

	// Copy primitive fields
	copy->legal_enpassant_squares = original->legal_enpassant_squares;
	copy->turn = original->turn;
	copy->fullmove_number = original->fullmove_number;

	for (int i = 0; i < 4; i++) {
		copy->castling_rights[i] = original->castling_rights[i];
	}

	// Allocate and copy Bitboard struct
	copy->board = malloc(sizeof(Gameboard));
	if (!copy->board) {
		free(copy);
		return NULL;
	}

	*(copy->board) = *(original->board); // struct copy

	return copy;
}

static Game_Tree_Node* _build_tree(const Game *game,
                                   Move in_move,
                                   int depth,
                                   U64 *out_count) {
    Game_Tree_Node *node = malloc(sizeof(*node));
    node->move = in_move;
    node->children = NULL;
    node->child_count = 0;
    node->count = 0;

    // Base case: depth == 0 → a leaf
    if (depth == 0) {
        node->count = 1;
        *out_count = 1;
        return node;
    }

    // Generate all legal moves from here
    int mv_count = 0;
	Move *mv_list = malloc(sizeof(Move) *MAX_MOVES);
    get_possible_moves(game, mv_list, &mv_count);
    if (mv_count == 0) {
        // no moves → also a leaf (e.g. checkmate or stalemate)
        node->count = 1;
        *out_count = 1;
        free(mv_list);
        return node;
    }

    // Allocate children array
    node->children = malloc(sizeof(*node->children) * mv_count);
    node->child_count = mv_count;

    Game *child_game;
    U64 subtotal, grand_total = 0;
    for (int i = 0; i < mv_count; i++) {
        // clone position, play the move
        child_game = copy_game(game);
        make_move(&mv_list[i], child_game);

        // recurse
        Game_Tree_Node *child_node = _build_tree(
            child_game, mv_list[i], depth - 1, &subtotal);

        // record
        node->children[i] = child_node;
        grand_total += subtotal;

        // cleanup
        free_game(child_game);
    }

    free(mv_list);

    node->count = grand_total;
    *out_count = grand_total;
    return node;
}

// Public entry point.  'game' is assumed to be at the root position, and
// total_count will be set to the perft(game, depth) value.
Game_Tree_Node* generate_game_tree(Game* game,
                                   int max_depth,
                                   int *total_count)
{
    // We pass a “dummy” move for the root; you can ignore it when printing.
    Move null_move = { .start = -1, .end = -1, .promotion = Empty };
    U64 count = 0;

    Game_Tree_Node *root = _build_tree(game, null_move,
                                       max_depth, &count);
    root->move = null_move;        // make sure root.move is “empty”
    *total_count = (int)count;
    return root;
}

void print_tree(const Game_Tree_Node *node, int depth) {
	for (int i = 0; i < node->child_count; i++) {
		const Game_Tree_Node *c = node->children[i];
		// indent
		for (int d = depth; d > 0; d--) {
			putchar(' ');
			putchar(' ');
		}
		// print move and count
		if (c->child_count == 0) {
			printf("%s\n",
			   move_to_uci(&c->move));
		}
		else {
			printf("%s: %llu\n",
			   move_to_uci(&c->move),
			   (unsigned long long)c->child_count);
		}

		// recurse
		print_tree(c, depth + 1);
	}
}




void free_game(Game* game) {
	free(game->board);
	free(game);
}

U64 perft(Game *game, const int depth) {
	if (depth == 0) return 1;

	int move_count = 0;
	Move* moves = malloc(sizeof(Move) * MAX_MOVES);
	get_possible_moves(game, moves, &move_count);
	get_eval(game);


	U64 nodes = 0;
	for (int i = 0; i < move_count; i++) {
		// 1) clone the position
		Game_State_Snapshot snapshot;
		save_game_state(game,&snapshot);

		// 2) make the move on the *clone*
		make_move(&moves[i], game);

		// 3) recurse into the clone
		nodes += perft(game, depth - 1);

		// 4) clean up
		load_game_state(game, &snapshot);
	}
	free(moves);
	return nodes;
}

void perft_divide(Game* game, int depth) {
	int move_count = 0;
	Move* moves = malloc(sizeof(Move) * MAX_MOVES);
	get_possible_moves(game, moves, &move_count);
	U64 total = 0;

	for (int i = 0; i < move_count; i++) {
		Game_State_Snapshot snapshot;
		save_game_state(game, &snapshot);

		make_move(&moves[i], game);
		U64 count = perft(game, depth-1);
		load_game_state(game, &snapshot);

		// Convert move to algebraic
		Square start_sq = number_to_square(moves[i].start);
		Square end_sq = number_to_square(moves[i].end);
		printf("%c%d%c%d: %" PRIu64 "\n",
			   file_to_char(start_sq.file), start_sq.rank,
			   file_to_char(end_sq.file), end_sq.rank,
			   count);

		total += count;
	}

	printf("Total: %" PRIu64 "\n", total);
	free(moves);
}

int algebraic_to_index(const char* square) {
	char file = square[0]; // 'a' to 'h'
	char rank = square[1]; // '1' to '8'

	if (file < 'a' || file > 'h' || rank < '1' || rank > '8')
		return -1;

	int file_index = file - 'a';      // 0–7
	int rank_index = rank - '1';      // 0–7
	return rank_index * 8 + file_index;
}

void make_move_str(const char* str, Game* game) {
	if (strlen(str) != 4 && strlen(str) != 6) {
		printf("Invalid move %s\n",str);
		return;
	}

	const char start_str[2] = {str[0],str[1]};
	const char end_str[2] = {str[2],str[3]};

	const int start = algebraic_to_index(start_str);
	const int end = algebraic_to_index(end_str);
	Piece promotion = Empty;

	if (start == -1 || end == -1) {
		printf("Invalid move: %s -> %s\n", start_str, end_str);
		return;
	}

	if ((strlen(str)==6 && str[4]=='=') && ((1ULL<<end)&RANK_1_8)) {
		promotion = char_to_piece(str[5]);
	}

	Move move = {
		.start = start,
		.end = end,
		.promotion = promotion  // You can add logic for promotion later
	};

	make_move(&move, game);
}

void make_moves_str(const char* str, Game* game) {
	// duplicate the input so we can tokenize safely
	char *temp = malloc(strlen(str) + 1);
	if (!temp) return;
	strcpy(temp, str);

	char *saveptr;
	char *token = strtok_r(temp, " ", &saveptr);
	while (token) {
		// expect UCI tokens like "e2e4"
		if (strlen(token) >= 4) {
			make_move_str(token, game);
		}
		else {
			printf("Invalid move: %s\n", token);
		}
		token = strtok_r(NULL, " ", &saveptr);
	}

	free(temp);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

bool set_board_from_fen(Game *game, const char *fen) {
    // make a mutable copy
    char *tmp = strdup(fen);
    if (!tmp) return false;

    char *saveptr = NULL;
    char *fields[6] = {0};

    // split into 6 space-separated tokens
    for (int i = 0; i < 6; i++) {
        if (i == 0)
            fields[i] = strtok_r(tmp, " \t\r\n", &saveptr);
        else
            fields[i] = strtok_r(NULL, " \t\r\n", &saveptr);
        if (!fields[i]) {
            free(tmp);
            return false;
        }
    }
    // any extra data is ignored

    Gameboard *b = game->board;
    // 1) clear all bitboards & state
    b->white_pieces = b->black_pieces = 0ULL;
    b->kings = b->queens = b->rooks = b->bishops = b->knights = b->pawns = 0ULL;
    game->turn = White;
    memset(game->castling_rights, 0, sizeof(game->castling_rights));
    game->legal_enpassant_squares = 0ULL;
    game->halfmove_clock   = 0;
    game->fullmove_number  = 1;

    // --- Field 1: piece placement ---
    const char *s = fields[0];
    int rank = 7, file = 0;
    while (*s && rank >= 0) {
        if (*s == '/') {
            rank--; file = 0;
        }
        else if (isdigit((unsigned char)*s)) {
            file += *s - '0';
        }
        else {
            if (file > 7) { free(tmp); return false; }
            int sq = rank*8 + file;
            U64 mask = 1ULL << sq;
            switch (*s) {
                case 'P': b->pawns   |= mask; b->white_pieces |= mask; break;
                case 'N': b->knights |= mask; b->white_pieces |= mask; break;
                case 'B': b->bishops |= mask; b->white_pieces |= mask; break;
                case 'R': b->rooks   |= mask; b->white_pieces |= mask; break;
                case 'Q': b->queens  |= mask; b->white_pieces |= mask; break;
                case 'K': b->kings   |= mask; b->white_pieces |= mask; break;
                case 'p': b->pawns   |= mask; b->black_pieces |= mask; break;
                case 'n': b->knights |= mask; b->black_pieces |= mask; break;
                case 'b': b->bishops |= mask; b->black_pieces |= mask; break;
                case 'r': b->rooks   |= mask; b->black_pieces |= mask; break;
                case 'q': b->queens  |= mask; b->black_pieces |= mask; break;
                case 'k': b->kings   |= mask; b->black_pieces |= mask; break;
                default:  free(tmp); return false;
            }
            file++;
        }
        s++;
    }
    if (rank != 0 || file != 8) { free(tmp); return false; }

    // --- Field 2: active color ---
    if (strcmp(fields[1], "w") == 0)      game->turn = White;
    else if (strcmp(fields[1], "b") == 0) game->turn = Black;
    else { free(tmp); return false; }

    // --- Field 3: castling rights ---
    if (strcmp(fields[2], "-") != 0) {
        for (char *c = fields[2]; *c; c++) {
            switch (*c) {
                case 'K': game->castling_rights[0] = true; break;
                case 'Q': game->castling_rights[1] = true; break;
                case 'k': game->castling_rights[2] = true; break;
                case 'q': game->castling_rights[3] = true; break;
                default:  free(tmp); return false;
            }
        }
    }

    // --- Field 4: en-passant target ---
    if (strcmp(fields[3], "-") != 0) {
        char f = fields[3][0] - 'a';
        char r = fields[3][1] - '1';
        if (f < 0 || f > 7 || r < 0 || r > 7) { free(tmp); return false; }
        int sq = r*8 + f;
        game->legal_enpassant_squares = 1ULL << sq;
    }

    // --- Field 5: halfmove clock ---
    game->halfmove_clock = (int)strtol(fields[4], NULL, 10);

    // --- Field 6: fullmove number ---
    game->fullmove_number = (int)strtol(fields[5], NULL, 10);

    free(tmp);
    return true;
}

Game* init_startpos_game() {
	Game* game = init_game();
	set_standard_board(game->board);
	return game;
}