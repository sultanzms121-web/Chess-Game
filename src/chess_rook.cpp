#include "chess_rook.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

using namespace godot;

void ChessRook::_bind_methods() {}

ChessRook::ChessRook()  {}
ChessRook::~ChessRook() {}

// Rook: any number of squares along a rank or file
bool ChessRook::is_valid_move(int from_col, int from_row, int to_col, int to_row) {
    if (from_col == to_col && from_row == to_row) return false;
    return (from_col == to_col || from_row == to_row);
}

std::vector<std::pair<int,int>> ChessRook::get_valid_moves(int col, int row) {
    std::vector<std::pair<int,int>> moves;

    // 4 directions: N, S, E, W
    const int dirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};

    for (auto &d : dirs) {
        for (int step = 1; step <= 7; step++) {
            int nc = col + d[0] * step;
            int nr = row + d[1] * step;
            if (!in_bounds(nc, nr)) break;
            moves.push_back({nc, nr});
        }
    }
    return moves;
}
