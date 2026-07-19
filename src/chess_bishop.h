#ifndef CHESS_BISHOP_H
#define CHESS_BISHOP_H

#include "chess_piece.h"

namespace godot {

class ChessBishop : public ChessPiece {
    GDCLASS(ChessBishop, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessBishop();
    ~ChessBishop();

    PieceType get_type() const override { return PieceType::BISHOP; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_BISHOP_H
