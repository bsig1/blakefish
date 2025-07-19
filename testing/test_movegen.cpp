//
// Created by blake on 7/19/25.
//
extern "C" {
#include "movegen.h"
#include "eval.h"
#include "game.h" // Your C structs like Game, Move, etc.
}

#include <gtest/gtest.h>

TEST(MoveGenTest, StartingPositionMoveCount) {
    Game game;
    init_game(&game); // Your C setup
    set_standard_board(game.board); // assuming this is how you initialize

    int move_count;
    Move* moves = generate_legal_moves(&game, &move_count);

    EXPECT_EQ(move_count, 20);  // In the starting position
}