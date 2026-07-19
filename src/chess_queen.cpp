#include "chess_queen.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

using namespace godot;

void ChessQueen::_bind_methods() {}

ChessQueen::ChessQueen()  {}
ChessQueen::~ChessQueen() {}

// Queen: any number of squares along a rank, file, or diagonal
bool ChessQueen::is_valid_move(int from_col, int from_row, int to_col, int to_row) {
    if (from_col == to_col && from_row == to_row) return false;

    int dc = std::abs(to_col - from_col);
    int dr = std::abs(to_row - from_row);

    // Same column, same row, or perfect diagonal
    return (dc == 0 || dr == 0 || dc == dr);
}

std::vector<std::pair<int,int>> ChessQueen::get_valid_moves(int col, int row) {
    std::vector<std::pair<int,int>> moves;

    // 8 directions: N, S, E, W, NE, NW, SE, SW
    const int dirs[8][2] = {
        {0,1},{0,-1},{1,0},{-1,0},{1,1},{-1,1},{1,-1},{-1,-1}
    };

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
