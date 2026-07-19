#ifndef CHESS_PAWN_H
#define CHESS_PAWN_H

#include "chess_piece.h"

namespace godot {

class ChessPawn : public ChessPiece {
    GDCLASS(ChessPawn, ChessPiece)

protected:
    static void _bind_methods();

public:
    ChessPawn();
    ~ChessPawn();

    PieceType get_type() const override { return PieceType::PAWN; }
    bool is_valid_move(int from_col, int from_row, int to_col, int to_row) override;
    std::vector<std::pair<int,int>> get_valid_moves(int col, int row) override;
};

}

#endif // CHESS_PAWN_H
