#include "chess_knight.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

using namespace godot;

void ChessKnight::_bind_methods() {}

ChessKnight::ChessKnight()  {}
ChessKnight::~ChessKnight() {}

// Knight: L-shape — 2 squares one axis + 1 square the other
bool ChessKnight::is_valid_move(int from_col, int from_row, int to_col, int to_row) {
    int dc = std::abs(to_col - from_col);
    int dr = std::abs(to_row - from_row);
    return (dc == 2 && dr == 1) || (dc == 1 && dr == 2);
}

std::vector<std::pair<int,int>> ChessKnight::get_valid_moves(int col, int row) {
    std::vector<std::pair<int,int>> moves;

    const int jumps[8][2] = {
        {-2,-1},{-2,1},{-1,-2},{-1,2},
        { 1,-2},{ 1,2},{ 2,-1},{ 2,1}
    };

    for (auto &j : jumps) {
        int nc = col + j[0];
        int nr = row + j[1];
        if (in_bounds(nc, nr)) {
            moves.push_back({nc, nr});
        }
    }
    return moves;
}
