//
// Created by mrsig on 7/11/2025.
//

#include <string.h>
#include <time.h>
#include "eval.h"

#include <stdio.h>

#include "bitboards.h"


static U64 rng_state;

void init_passed_masks(void) {
    for (int sq = 0; sq < 64; sq++) {
        const int file = sq & 7; // file 0..7
        const int rank = sq >> 3; // rank 0..7
        U64 mask = 0ULL;

        // white: ranks r+1…7
        for (int rr = rank + 1; rr < 8; rr++)
            for (int ff = file - 1; ff <= file + 1; ff++)
                if (ff >= 0 && ff < 8)
                    mask |= 1ULL << (rr * 8 + ff);

        passed_mask_white[sq] = mask;

        // black: ranks 0…r-1
        mask = 0ULL;
        for (int rr = 0; rr < rank; rr++)
            for (int ff = file - 1; ff <= file + 1; ff++)
                if (ff >= 0 && ff < 8)
                    mask |= 1ULL << (rr * 8 + ff);

        passed_mask_black[sq] = mask;
    }
}


void init_outpost_masks() {
    for (int square = 0; square < 64; ++square) {
        const int file = square % 8;
        const int rank = square / 8;

        outpost_attackers[White][square] = 0ULL;
        outpost_attackers[Black][square] = 0ULL;
        outpost_defenders[White][square] = 0ULL;
        outpost_defenders[Black][square] = 0ULL;

        // Knight outpost immunity: cannot be attacked by enemy pawns

        // For White outposts: enemy black pawns attack from ahead (rank +1)
        if (rank <= 5) {
            if (file > 0) outpost_attackers[White][square] |= 1ULL << (square + 7);
            if (file < 7) outpost_attackers[White][square] |= 1ULL << (square + 9);
        }

        // For Black outposts: enemy white pawns attack from behind (rank -1)
        if (rank >= 2) {
            if (file > 0) outpost_attackers[Black][square] |= 1ULL << (square - 9);
            if (file < 7) outpost_attackers[Black][square] |= 1ULL << (square - 7);
        }

        // Knight guard squares: protected by friendly pawn from behind

        // White pawns guard from rank -1
        if (rank > 0)
            outpost_defenders[White][square] = 1ULL << (square - 8);

        // Black pawns guard from rank +1
        if (rank < 7)
            outpost_defenders[Black][square] = 1ULL << (square + 8);
    }
}

void init_eval() {
    init_passed_masks();
    init_outpost_masks();
}


// Call once at startup:
static void seed_rng(void) {
    rng_state = (uint64_t) time(NULL) ^ 0xdeadbeefcafebabeULL;
}

// xor shift random generator 64bit
static U64 rand64(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 2685821657736338717ULL;
}

// Usage in initPawnHash():
void initPawnHash(void) {
    seed_rng();
    for (int c = 0; c < 2; c++)
        for (int sq = 0; sq < 64; sq++)
            pawnKeys[c][sq] = rand64();
}

// ——— Initialization ———
void init_pawn_hashtable(void) {
    // zero the table
    memset(pawnTable, 0, sizeof(pawnTable));
    for (int c = 0; c < 2; c++)
        for (int sq = 0; sq < 64; sq++)
            pawnKeys[c][sq] = rand64();
    pawnHashKey = 0ULL;
}

// ——— Incrementally update on pawn moves/captures ———
// side: 0 = White, 1 = Black
void updatePawnKey(const int side, const int from_sq, const int to_sq, const int capture_sq) {
    pawnHashKey ^= pawnKeys[side][from_sq]; // remove pawn from from_sq
    pawnHashKey ^= pawnKeys[side][to_sq]; // add pawn to to_sq
    if (capture_sq >= 0) {
        const int enemy = side ^ 1;
        pawnHashKey ^= pawnKeys[enemy][capture_sq]; // remove captured pawn
    }
}

// ——— Pawn‐hash lookup/store ———
static int lookupPawnHash(int *outEval) {
    const size_t idx = pawnHashKey & PAWN_HASH_MASK;
    if (pawnTable[idx].key == pawnHashKey) {
        *outEval = pawnTable[idx].eval;
        return 1; // hit
    }
    return 0; // miss
}

static void storePawnHash(int eval) {
    const size_t idx = pawnHashKey & PAWN_HASH_MASK;
    pawnTable[idx].key = pawnHashKey;
    pawnTable[idx].eval = (int16_t) eval;
}

static int count_isolated_pawns(const U64 pawns) {
    // mask off wrap-around bits if you’re shifting across file A H
    const U64 left_pawns = (pawns << 1) & (~FILE_H);
    const U64 right_pawns = (pawns >> 1) & (~FILE_A);
    const U64 isolations = pawns & ~(left_pawns | right_pawns);
    return popcount(isolations);
}

static int count_doubled_pawns(const U64 pawns) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        count += popcount(pawns & FILE_MASKS[i]) - 1;
    }
    return count;
}

static int count_backward_white(const U64 whitePawns, const U64 blackPawns) {
    // 1. find white pawns lacking a “support” pawn on f±1, rank+1
    const U64 supportNW = (whitePawns << 7) & ~FILE_H; // front-left
    const U64 supportNE = (whitePawns << 9) & ~FILE_A; // front-right
    const U64 unsupported = whitePawns & ~(supportNW | supportNE);

    // 2. build the attack map of black pawns onto the rank below them
    //    black pawns attack south-west (>>9) and south-east (>>7)
    const U64 attackSW = (blackPawns & ~FILE_A) >> 9;
    const U64 attackSE = (blackPawns & ~FILE_H) >> 7;
    const U64 blackAttacks = attackSW | attackSE;

    // 3. which of our unsupported pawns have their forward square under fire?
    //    shift that attack map “down” one rank (>>8) to align with the pawn
    const U64 attackedAhead = blackAttacks >> 8;

    // 4. backward pawns are exactly those unsupported pawns whose front
    //    square is attacked
    const U64 backward = unsupported & attackedAhead;

    return popcount(backward);
}

static int count_backward_black(const U64 blackPawns, const U64 whitePawns) {
    // no friendly pawn supporting from behind (rank−1, f±1)
    const U64 supportSW = (blackPawns >> 7) & ~FILE_A; // back-right (from Black’s POV)
    const U64 supportSE = (blackPawns >> 9) & ~FILE_H; // back-left
    const U64 unsupported = blackPawns & ~(supportSW | supportSE);

    // white pawn attack map (north-west and north-east)
    const U64 attackNW = (whitePawns & ~FILE_H) << 7;
    const U64 attackNE = (whitePawns & ~FILE_A) << 9;
    const U64 whiteAttacks = attackNW | attackNE;

    // align attacks with the black pawn’s square by shifting <<8
    const U64 attackedAhead = whiteAttacks << 8;

    const U64 backward = unsupported & attackedAhead;
    return popcount(backward);
}

static int score_passed_white(const U64 whitePawns, const U64 blackPawns) {
    int score = 0;
    U64 wp = whitePawns;
    while (wp) {
        const int sq = pop_lsb(&wp);
        if (!(blackPawns & passed_mask_white[sq])) {
            score += passed_bonus[White][sq >> 3];
        }
    }
    return score;
}

static int score_passed_black(const U64 blackPawns, const U64 whitePawns) {
    int score = 0;
    U64 bp = blackPawns;
    while (bp) {
        const int sq = pop_lsb(&bp);
        if (!(whitePawns & passed_mask_black[sq])) {
            score += passed_bonus[Black][sq >> 3];
        }
    }
    return score;
}

static int count_connected_pawns(const U64 pawns) {
    const U64 chainLinks = (pawns << 7) | (pawns << 9);
    return popcount(chainLinks & pawns);
}

static int count_pawn_islands(const U64 pawns) {
    bool has_pawn[8] = {};

    for (int i = 0; i < 8; i++) {
        has_pawn[i] = FILE_MASKS[i] & pawns;
    }

    int islands = 0;
    for (int f = 0; f < 8; f++) {
        if (has_pawn[f] && (f == 0 || !has_pawn[f - 1]))
            islands++;
    }
    return islands;
}

static int score_pawn_structure(const Gameboard *board) {
    int cached;
    if (lookupPawnHash(&cached))
        return cached;

    const U64 white_pawns = board->white_pieces & board->pawns;
    const U64 black_pawns = board->black_pieces & board->pawns;
    int eval = 0;
    // 1. isolated
    eval -= count_isolated_pawns(white_pawns) * ISO_PEN;
    eval += count_isolated_pawns(black_pawns) * ISO_PEN;
    // 2. doubled
    eval -= count_doubled_pawns(white_pawns) * DOUBLED_PEN;
    eval += count_doubled_pawns(black_pawns) * DOUBLED_PEN;
    // 3. backward
    eval -= count_backward_black(black_pawns, white_pawns) * BACKWARD_PEN;
    eval += count_backward_white(white_pawns, black_pawns) * BACKWARD_PEN;
    // 4. passed
    eval -= score_passed_black(black_pawns, white_pawns);
    eval += score_passed_white(white_pawns, black_pawns);
    // 5. chains
    eval += count_connected_pawns(white_pawns) * CHAIN_BONUS;
    eval -= count_connected_pawns(black_pawns) * CHAIN_BONUS;
    // 6. islands
    eval -= (count_pawn_islands(white_pawns) - 1) * ISLAND_PEN;
    eval += (count_pawn_islands(black_pawns) - 1) * ISLAND_PEN;

    storePawnHash(eval);
    return eval;
}

static int score_material(const Gameboard *board, int *total_material) {
    int white = 0;
    int black = 0;
    white += popcount(board->bishops & board->white_pieces) * BISHOP_VALUE;
    if (white > BISHOP_VALUE) {
        white += BISHOP_PAIR_BONUS;
    }
    white += popcount(board->knights & board->white_pieces) * KNIGHT_VALUE;
    white += popcount(board->rooks & board->white_pieces) * ROOK_VALUE;
    white += popcount(board->queens & board->white_pieces) * QUEEN_VALUE;
    white += popcount(board->pawns & board->white_pieces) * PAWN_VALUE;

    black += popcount(board->bishops & board->black_pieces) * BISHOP_VALUE;
    if (black > BISHOP_VALUE) {
        black += BISHOP_PAIR_BONUS;
    }
    black += popcount(board->knights & board->black_pieces) * KNIGHT_VALUE;
    black += popcount(board->rooks & board->black_pieces) * ROOK_VALUE;
    black += popcount(board->queens & board->black_pieces) * QUEEN_VALUE;
    black += popcount(board->pawns & board->black_pieces) * PAWN_VALUE;
    *total_material = white + black;
    return white - black;
}


static int score_knights(U64 bitboard, const U64 enemy_pawns, const U64 friendly_pawns, const int color,
                         const int (*PST)[2][6][64]) {
    int score = 0;
    int square = 0;

    while (bitboard) {
        square = pop_lsb(&bitboard);
        score += (*PST)[color][Knight][square];
        if (outpost_attackers[color][square] & (~enemy_pawns) && outpost_defenders[color][square] & friendly_pawns) {
            score += outpost_bonus[color][square];
        }
    }
    return score;
}

static int score_rooks(U64 bitboard, const U64 enemy_pawns, const U64 friendly_pawns, const int color,
                       const int (*PST)[2][6][64]) {
    int score = 0;
    int square = 0;

    while (bitboard) {
        square = pop_lsb(&bitboard);
        score += (*PST)[color][Rook][square];
        if (color == White) {
            const U64 file = FILE_MASKS[square & 7]; // &7 is equivalent to %8
            if (!(friendly_pawns & file)) {
                score += SEMI_OPEN_FILE_BONUS;
                if (!(enemy_pawns & file)) {
                    score += OPEN_FILE_BONUS;
                }
            }
        }
    }
    return score;
}

static int score_bishops(U64 bitboard, const U64 enemy_pawns, const U64 friendly_pawns, const int color,
                         const int (*PST)[2][6][64]) {
    int score = 0;
    int square = 0;

    while (bitboard) {
        square = pop_lsb(&bitboard);
        score += (*PST)[color][Bishop][square];
        if (outpost_attackers[color][square] & (~enemy_pawns) && outpost_defenders[color][square] & friendly_pawns) {
            score += outpost_bonus[color][square];
        }
    }
    return score;
}

static int score_queens(U64 bitboard, const int color, const int (*PST)[2][6][64]) {
    int score = 0;
    int square = 0;

    while (bitboard) {
        square = pop_lsb(&bitboard);
        score += (*PST)[color][Queen][square];
    }
    return score;
}

static int score_pawns(U64 bitboard, const Color color, const int (*PST)[2][6][64]) {
    int score = 0;
    while (bitboard) {
        const int square = pop_lsb(&bitboard);
        score += (*PST[color][Pawn][square]);
    }
    return score;
}

static int score_kings(U64 bitboard, const U64 friendly_pawns, const Color color,
                       const int (*PST)[2][6][64]) {
    int score = 0;
    while (bitboard) {
        const int square = pop_lsb(&bitboard);
        score += (*PST[color][Pawn][square]);
        score += popcount(king_moves[square] & friendly_pawns);
    }

    return score;
}

static int score_pieces_middlegame(const Gameboard *board) {
    int white = 0;
    int black = 0;
    const int (*PST)[2][6][64] = &piece_square_table_middlegame;


    // Bishops
    white += score_bishops(board->bishops & board->white_pieces, board->pawns & board->black_pieces,
                           board->pawns & board->white_pieces, White, PST);
    black += score_bishops(board->bishops & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Knights
    white += score_knights(board->knights & board->white_pieces, board->pawns & board->black_pieces,
                       board->pawns & board->white_pieces, White, PST);
    black += score_knights(board->knights & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Rooks
    white += score_rooks(board->rooks & board->white_pieces, board->pawns & board->black_pieces,
                        board->pawns & board->white_pieces, White, PST);
    black += score_rooks(board->rooks & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Queens
    white += score_queens(board->queens & board->white_pieces,  White, PST);
    black += score_queens(board->queens & board->black_pieces, Black, PST);

    // Pawns
    white += score_pawns(board->pawns & board->white_pieces,  White, PST);
    black += score_pawns(board->pawns & board->black_pieces, Black, PST);

    // Kings
    white += score_kings(board->kings & board->white_pieces,board->pawns & board->white_pieces, White, PST);
    white += score_kings(board->kings & board->black_pieces,board->pawns & board->black_pieces, White, PST);


    return white - black;
}

static int score_pieces_endgame(const Gameboard *board) {
    int white = 0;
    int black = 0;
    const int (*PST)[2][6][64] = &piece_square_table_endgame;


    // Bishops
    white += score_bishops(board->bishops & board->white_pieces, board->pawns & board->black_pieces,
                           board->pawns & board->white_pieces, White, PST);
    black += score_bishops(board->bishops & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Knights
    white += score_knights(board->knights & board->white_pieces, board->pawns & board->black_pieces,
                       board->pawns & board->white_pieces, White, PST);
    black += score_knights(board->knights & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Rooks
    white += score_rooks(board->rooks & board->white_pieces, board->pawns & board->black_pieces,
                        board->pawns & board->white_pieces, White, PST);
    black += score_rooks(board->rooks & board->black_pieces, board->pawns & board->white_pieces,
                           board->pawns & board->black_pieces, Black, PST);
    // Queens
    white += score_queens(board->queens & board->white_pieces,  White, PST);
    black += score_queens(board->queens & board->black_pieces, Black, PST);

    // Pawns
    white += score_pawns(board->pawns & board->white_pieces,  White, PST);
    black += score_pawns(board->pawns & board->black_pieces, Black, PST);

    // Kings
    white += score_kings(board->kings & board->white_pieces,board->pawns & board->white_pieces, White, PST);
    white += score_kings(board->kings & board->black_pieces,board->pawns & board->black_pieces, White, PST);


    return white - black;
}

int get_eval(const Game *game) {
    // Returns evaluation in centi-pawns
    int eval_score = 0;
    int total_material = 0;

    eval_score += score_material(game->board, &total_material);
    if (total_material < ENDGAME_MATERIAL_LINE) {
        //This is an endgame... ish
        eval_score += score_pieces_endgame(game->board);
    } else {
        eval_score += score_pieces_middlegame(game->board);
    }

    eval_score += score_pawn_structure(game->board);
    eval_score += (game->turn == White ? +TURN_BONUS : -TURN_BONUS);



    return eval_score;
}

void print_eval(const Game *game,const int depth) {
    printf("Current evaluation: %.2f\n",(double)get_eval(game)/100);
}
