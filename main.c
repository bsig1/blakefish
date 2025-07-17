#include <time.h>
#include <stdio.h>

#include "eval.h"
#include "game_tree.h"


double now() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + ts.tv_nsec*1e-9;
}

void print_perft(Game *game, const Bitboards *bb, const int depth) {
	const double t0 = now();
	const U64 nodes = perft(game, bb, depth);
	const double t1 = now();

	const double secs = t1 - t0;
	printf("Perft(%d) = %llu in %.3f s -> %.0f knps\n",
		depth, nodes, secs, (double)nodes/1000.0/secs);
}

void print_eval(const Game *game, const Bitboards *bb, const int depth) {
	printf("Current evaluation: %d\n",get_eval(game));
}
static void emit_immune_masks(void) {
	// White knights: enemy Black pawns on adjacent files at ANY rank > r_knight
	printf("static const U64 KNIGHT_OUTPOST_IMMUNE_WHITE[64] = {\n    ");
	for (int sq = 0; sq < 64; ++sq) {
		int r = sq / 8, f = sq % 8;
		U64 m = 0ULL;
		// for each adjacent file
		if (f > 0) {
			for (int r2 = r + 1; r2 < 8; ++r2)
				m |= 1ULL << (r2*8 + (f-1));
		}
		if (f < 7) {
			for (int r2 = r + 1; r2 < 8; ++r2)
				m |= 1ULL << (r2*8 + (f+1));
		}
		printf("0x%016llxULL%s", m,
			   (sq % 4 == 3 && sq != 63) ? ",\n    " : " ");
	}
	printf("\n};\n\n");

	// Black knights: enemy White pawns on adjacent files at ANY rank < r_knight
	printf("static const U64 KNIGHT_OUTPOST_IMMUNE_BLACK[64] = {\n    ");
	for (int sq = 0; sq < 64; ++sq) {
		int r = sq / 8, f = sq % 8;
		U64 m = 0ULL;
		if (f > 0) {
			for (int r2 = 0; r2 < r; ++r2)
				m |= 1ULL << (r2*8 + (f-1));
		}
		if (f < 7) {
			for (int r2 = 0; r2 < r; ++r2)
				m |= 1ULL << (r2*8 + (f+1));
		}
		printf("0x%016llxULL, %s", m,
			   (sq % 4 == 3 && sq != 63) ? "\n    " : "");
	}
	printf("\n};\n");
}

static void emit_guard_mask(int color) {
	// color==0 → White knight needs pawns on rank-1; color==1 → Black knight needs pawns on rank+1
	const char *name = color==0
	  ? "KNIGHT_OUTPOST_GUARD_WHITE"
	  : "KNIGHT_OUTPOST_GUARD_BLACK";
	int dr = (color==0) ? -1 : +1;

	printf("static const U64 %s[64] = {\n    ", name);
	for (int sq = 0; sq < 64; ++sq) {
		int r = sq / 8, f = sq % 8;
		U64 m = 0ULL;
		int r_from = r + dr;
		if (r_from >= 0 && r_from < 8) {
			if (f >  0) m |= 1ULL << (r_from*8 + (f-1));
			if (f <  7) m |= 1ULL << (r_from*8 + (f+1));
		}
		printf("0x%016llxULL%s", m,
			   (sq % 4 == 3 && sq != 63) ? ",\n    " : ", ");
	}
	printf("\n};\n\n");
}

int main(void) {
	emit_immune_masks();
	emit_guard_mask(0); // White
	emit_guard_mask(1); // Black
	return 0;
}

//int main() {

	/*
	Game *game = init_startpos_game();
	Bitboards *bb = init_bitboards();

	make_moves_str("",game);
	print_eval(game, bb,0);
	//print_perft(game,bb,5);

	*/



	/*

*/
//	return 0;
//}