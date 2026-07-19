#include "chess_board.h"
#include "chess_constants.h"
#include "game_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

using namespace godot;

void ChessBoard::_bind_methods() {
    ClassDB::bind_method(D_METHOD("on_board_click", "camera", "event", "position", "normal", "shape_idx"), &ChessBoard::on_board_click);
}

ChessBoard::ChessBoard() {}
ChessBoard::~ChessBoard() {}

void ChessBoard::on_board_click(Node *camera, const Ref<InputEvent> &event, const Vector3 &position, const Vector3 &normal, int shape_idx) {
    (void)camera; (void)normal; (void)shape_idx; // fixed by Area3D.input_event's signal signature
    InputEventMouseButton *mouse_event = Object::cast_to<InputEventMouseButton>(event.ptr());

    if (mouse_event && mouse_event->is_pressed() && mouse_event->get_button_index() == MOUSE_BUTTON_LEFT) {
        int col = x_to_col(position.x);
        int row = z_to_row(position.z);

        GameManager *gm = GameManager::get_singleton();
        if (gm) {
            gm->handle_square_click(col, row);
        }
    }
}
