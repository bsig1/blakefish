#include <time.h>
#include <stdio.h>

#include "eval.h"
#include "game_tree.h"


double now() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec*1e-9;
}

void print_perft(Game *game, const int depth) {
	const double t0 = now();
	const U64 nodes = perft(game, depth);
	const double t1 = now();

	const double secs = t1 - t0;
	printf("Perft(%d) = %llu in %.3f s -> %.0f knps\n",
		depth, nodes, secs, (double)nodes/1000.0/secs);
}



int main() {


	Game *game = init_startpos_game();
	init_bitboards();
	set_board_from_fen(game,"r1b2rk1/pp3ppp/2n1pn2/1B1q4/3P4/2N1PN2/PP3PPP/R2Q1RK1 w - - 0 12");
	print_board(game->board);
	print_eval(game,0);
	int total_count = 0;
	Game_Tree_Node *root = generate_game_tree(game,4,&total_count);
	print_tree(root,0);

	return 0;
}
