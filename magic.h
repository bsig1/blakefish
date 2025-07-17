#ifndef MAGIC_H
#define MAGIC_H

#include <stdint.h>

typedef uint64_t U64;

extern const U64 rook_magics[64];
extern const U64 bishop_magics[64];
extern const int rook_shifts[64];
extern const int bishop_shifts[64];
extern const U64 rook_masks[64];
extern const U64 bishop_masks[64];

// Magic bitboard utilities
U64 set_blockers(int index, U64 mask);
U64 random_magic_candidate(void);
U64 find_magic_number(int square, U64 (*attack_on_the_fly)(U64, U64), U64 mask);

U64** build_magic_attack_table(const U64 magic_numbers[64],const int shifts[64],
							   U64 (*generate_mask)(int),
							   U64 (*attack_on_the_fly)(U64, U64),
							   int table_sizes[64]);
void free_magic_attack_table(U64** table, int table_lengths);

#endif // MAGIC_H