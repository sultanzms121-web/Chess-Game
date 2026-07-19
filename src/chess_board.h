#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {

class ChessBoard : public Node3D {
    GDCLASS(ChessBoard, Node3D)

protected:
    static void _bind_methods();

public:
    ChessBoard();
    ~ChessBoard();

    // The board's own click detector (the "glass shield" Area3D covering
    // the whole playable surface). Clicking an empty square comes through
    // here; clicking directly on a piece comes through ChessPiece::on_click
    // instead — both funnel into GameManager::handle_square_click.
    void on_board_click(Node *camera, const Ref<InputEvent> &event, const Vector3 &position, const Vector3 &normal, int shape_idx);
};

}

#endif // CHESS_BOARD_H
