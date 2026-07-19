#ifndef CHESS_QUEEN_H
#define CHESS_QUEEN_H

#include "chess_piece.h"

namespace godot {

class ChessQueen : public ChessPiece {
    GDCLASS(ChessQueen, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessQueen();
    ~ChessQueen();

    PieceType get_type() const override { return PieceType::QUEEN; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_QUEEN_H
