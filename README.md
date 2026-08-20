# Blakefish

Blakefish is a chess engine written from scratch in C.

The project focuses on fast, correct move generation using bitboards and low-level board representations. It includes full legal move generation, sliding-piece attacks using magic bitboards, make/unmake support, and a basic evaluation/search framework.

## Features

- 64-bit bitboard board representation
- Magic bitboards for bishop and rook attacks
- Precomputed king, knight, and pawn attack tables
- Fully legal move generation
  - Checks and double checks
  - Pins
  - Castling
  - En passant
  - Promotions
- Make and undo move support
- Castling-right and en-passant state tracking
- Position evaluation
  - Material
  - Piece-square tables
  - Pawn structure
  - Rook open/semi-open files
  - Knight outposts
  - Tempo
- Perft testing for move-generation correctness
- Move-generation performance of approximately **10 million moves per second**

## Architecture

Blakefish represents the board using a collection of 64-bit bitboards.

Each bit corresponds to one square on the chessboard, allowing many board operations to be implemented using bitwise arithmetic rather than iterating over individual squares.

Sliding-piece attacks are generated using **magic bitboards**, which map blocker configurations to precomputed attack sets for bishops and rooks.

Legal move generation handles king safety directly, including pinned pieces and check-evasion masks, rather than generating every pseudo-legal move and testing each resulting position independently.

## Move Generation

The engine handles all standard chess rules, including:

- Pawn pushes and captures
- Double pawn pushes
- Knight moves
- Bishop, rook, and queen sliding attacks
- King moves
- Castling
- En passant
- Promotions
- Single-check evasions
- Double-check positions
- Absolute pins

Special attention is given to edge cases such as en passant exposing the king to a rook or queen attack.

## Perft

Move-generation correctness is tested using **perft**, which recursively enumerates all legal positions reachable to a given depth.

This provides a useful way to verify complicated rules such as:

- Castling legality
- En passant
- Checks
- Pins
- Promotions
- Make/undo correctness

Perft testing was used throughout development to compare Blakefish against known chess move-generation results and isolate legality bugs.

## Evaluation

The engine includes a hand-written static evaluation function using features such as:

- Material balance
- Piece-square tables
- Pawn structure
- Open and semi-open files for rooks
- Knight outposts
- Side-to-move tempo

The primary focus of the project has been move generation, correctness, and engine architecture rather than playing strength.

## Building

Blakefish uses CMake.

```bash
git clone https://github.com/bsig1/blakefish.git
cd blakefish

cmake -S . -B build
cmake --build build
````

## Testing

The project includes automated tests for engine components and move-generation correctness.

After building:

```bash
ctest --test-dir build
```

Perft tests can also be used to validate the move generator against known positions.

## Goals

Blakefish is primarily an exploration of:

* High-performance C
* Bit-level data structures
* Search algorithms
* Chess programming
* Performance profiling and optimization
* Testing highly stateful systems for correctness

The project is intentionally implemented at a relatively low level to better understand the techniques used in modern chess engines.

```
```
