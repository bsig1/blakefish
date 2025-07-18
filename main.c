#include <time.h>
#include <stdio.h>

#include "eval.h"
#include "game_tree.h"


double now() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + ts.tv_nsec*1e-9;
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
	make_moves_str("",game);
	print_eval(game,0);
	print_perft(game,5);

	return 0;
}