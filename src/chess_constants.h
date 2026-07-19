#ifndef CHESS_CONSTANTS_H
#define CHESS_CONSTANTS_H

#include <cmath>

// ─── Board geometry ────────────────────────────────────────────────────
// new_board.glb, native scale 1.0x (no scene transform applied):
//   Outer board frame   (ChessBoard.002): -0.31166..0.31166 -> 0.62333 m
//   Inner playable grid (ChessBoard.001): -0.24933..0.24933 -> 0.49866 m
// The 8x8 checker squares are painted onto the INNER grid mesh, so the
// square size must be derived from the inner grid, not the outer frame.
const float PLAYABLE_AREA = 0.49866f;
const float SQUARE_SIZE   = PLAYABLE_AREA / 8.0f;    // 0.0623325 m per square
const float BOARD_HALF    = PLAYABLE_AREA / 2.0f;    // 0.24933 m
const float CELL_HALF     = SQUARE_SIZE  / 2.0f;     // 0.03117 m
const float LIFT_HEIGHT   = 0.05f;                    // 5 cm lift on select
// Height of the playing surface (top of the inner grid mesh) above the
// board's own origin — this is where every piece's base should rest.
const float BOARD_SURFACE_Y = 0.000206f;

// ─── Grid <-> world helpers ───────────────────────────────────────────
inline float col_to_x(int col) { return -BOARD_HALF + col * SQUARE_SIZE + CELL_HALF; }
inline float row_to_z(int row) { return -BOARD_HALF + row * SQUARE_SIZE + CELL_HALF; }

inline int x_to_col(float x) {
    int c = static_cast<int>(std::floor((x + BOARD_HALF) / SQUARE_SIZE));
    if (c < 0) c = 0;
    if (c > 7) c = 7;
    return c;
}
inline int z_to_row(float z) {
    int r = static_cast<int>(std::floor((z + BOARD_HALF) / SQUARE_SIZE));
    if (r < 0) r = 0;
    if (r > 7) r = 7;
    return r;
}

inline bool in_bounds(int col, int row) { return col >= 0 && col <= 7 && row >= 0 && row <= 7; }

#endif // CHESS_CONSTANTS_H
