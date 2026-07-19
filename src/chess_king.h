#ifndef CHESS_KING_H
#define CHESS_KING_H

#include "chess_piece.h"

namespace godot {

class ChessKing : public ChessPiece {
    GDCLASS(ChessKing, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessKing();
    ~ChessKing();

    PieceType get_type() const override { return PieceType::KING; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_KING_H
