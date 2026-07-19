#ifndef CHESS_KNIGHT_H
#define CHESS_KNIGHT_H

#include "chess_piece.h"

namespace godot {

class ChessKnight : public ChessPiece {
    GDCLASS(ChessKnight, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessKnight();
    ~ChessKnight();

    PieceType get_type() const override { return PieceType::KNIGHT; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_KNIGHT_H
