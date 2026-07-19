#ifndef CHESS_ROOK_H
#define CHESS_ROOK_H

#include "chess_piece.h"

namespace godot {

class ChessRook : public ChessPiece {
    GDCLASS(ChessRook, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessRook();
    ~ChessRook();

    PieceType get_type() const override { return PieceType::ROOK; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_ROOK_H
