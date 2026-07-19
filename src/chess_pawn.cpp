#include "chess_pawn.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

using namespace godot;

void ChessPawn::_bind_methods() {}

ChessPawn::ChessPawn()  {}
ChessPawn::~ChessPawn() {}

// Pawn: 1 forward (2 from start row), same column forward moves only, but 1-square diagonal
// moves are allowed when capturing or performing en-passant.
// White pawns advance +row, Black pawns advance −row
bool ChessPawn::is_valid_move(int from_col, int from_row, int to_col, int to_row) {
    int direction = (piece_color == "white") ? 1 : -1;
    int start_row = (piece_color == "white") ? 1 : 6;

    int dc = to_col - from_col;
    int forward = (to_row - from_row) * direction; // normalised: should be positive

    if (forward == 1) {
        if (dc == 0) return true;                // 1 step forward
        if (std::abs(dc) == 1) return true;      // 1 square diagonal capture / en-passant
    }
    if (forward == 2 && dc == 0 && from_row == start_row) return true;  // 2 steps from start
    return false;
}

std::vector<std::pair<int,int>> ChessPawn::get_valid_moves(int col, int row) {
    std::vector<std::pair<int,int>> moves;

    int direction = (piece_color == "white") ? 1 : -1;
    int start_row = (piece_color == "white") ? 1 : 6;

    // 1 square forward
    int nr1 = row + direction;
    if (in_bounds(col, nr1)) {
        moves.push_back({col, nr1});

        // Diagonal capture candidates
        if (in_bounds(col - 1, nr1)) moves.push_back({col - 1, nr1});
        if (in_bounds(col + 1, nr1)) moves.push_back({col + 1, nr1});

        // 2 squares forward from starting row (only if 1-forward is on board)
        if (row == start_row) {
            int nr2 = row + 2 * direction;
            if (in_bounds(col, nr2)) {
                moves.push_back({col, nr2});
            }
        }
    }

    return moves;
}
