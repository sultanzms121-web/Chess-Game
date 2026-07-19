#include "chess_king.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

using namespace godot;

void ChessKing::_bind_methods() {
    // Inherits all bindings from ChessPiece — nothing extra needed
}

ChessKing::ChessKing()  {}
ChessKing::~ChessKing() {}

// King: exactly 1 square in any direction (including diagonals)
bool ChessKing::is_valid_move(int from_col, int from_row, int to_col, int to_row) {
    int dc = std::abs(to_col - from_col);
    int dr = std::abs(to_row - from_row);
    return (dc <= 1 && dr <= 1) && !(dc == 0 && dr == 0);
}

std::vector<std::pair<int,int>> ChessKing::get_valid_moves(int col, int row) {
    std::vector<std::pair<int,int>> moves;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) continue;
            int nc = col + dx;
            int nr = row + dz;
            if (in_bounds(nc, nr)) {
                moves.push_back({nc, nr});
            }
        }
    }
    return moves;
}
