#include "chess_piece.h"
#include "chess_constants.h"
#include "game_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <cmath>

using namespace godot;

// ─── Binding ──────────────────────────────────────────────────────────
void ChessPiece::_bind_methods() {
    ClassDB::bind_method(D_METHOD("on_click", "camera", "event", "position", "normal", "shape_idx"), &ChessPiece::on_click);

    ClassDB::bind_method(D_METHOD("set_piece_color", "color"), &ChessPiece::set_piece_color);
    ClassDB::bind_method(D_METHOD("get_piece_color"),          &ChessPiece::get_piece_color);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "piece_color"), "set_piece_color", "get_piece_color");

    ClassDB::bind_method(D_METHOD("get_col"), &ChessPiece::get_col);
    ClassDB::bind_method(D_METHOD("get_row"), &ChessPiece::get_row);
    ClassDB::bind_method(D_METHOD("get_type_name"), &ChessPiece::get_type_name);
    ClassDB::bind_method(D_METHOD("set_selected_visual", "selected"), &ChessPiece::set_selected_visual);
}

// ─── Construction ─────────────────────────────────────────────────────
ChessPiece::ChessPiece() {
    is_selected_visual = false;
    is_initialized      = false;
    board_surface_y     = 0.0f;
    piece_col           = 0;
    piece_row           = 0;
    piece_color         = "white";
    has_moved           = false;
    is_captured         = false;
    target_rotation_y   = 0.0f;
    original_rotation_y = 0.0f;
    set_process(true);
}

ChessPiece::~ChessPiece() {}

// ─── Ready: snap grid coords immediately and register with GameManager ─
void ChessPiece::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;

    Vector3 start_pos = get_position();
    piece_col = x_to_col(start_pos.x);
    piece_row = z_to_row(start_pos.z);

    target_position = Vector3(col_to_x(piece_col), start_pos.y, row_to_z(piece_row));
    Vector3 start_rot = get_rotation();
    original_rotation_y = start_rot.y;
    target_rotation_y = original_rotation_y;
    
    board_surface_y = start_pos.y;
    is_initialized  = true;

    if (GameManager::get_singleton()) {
        GameManager::get_singleton()->register_piece(this, piece_col, piece_row);
    }
}

// ─── Per‑frame update ─────────────────────────────────────────────────
void ChessPiece::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) return;

    if (!is_initialized) {
        Vector3 start_pos = get_position();
        piece_col = x_to_col(start_pos.x);
        piece_row = z_to_row(start_pos.z);
        target_position = Vector3(col_to_x(piece_col), start_pos.y, row_to_z(piece_row));
        
        Vector3 start_rot = get_rotation();
        original_rotation_y = start_rot.y;
        target_rotation_y = original_rotation_y;
        
        board_surface_y = start_pos.y;
        is_initialized  = true;
    }

    Vector3 current_pos = get_position();
    set_position(current_pos.lerp(target_position, 15.0 * delta));
    
    Vector3 current_rot = get_rotation();
    current_rot.y = current_rot.y + (target_rotation_y - current_rot.y) * (15.0 * delta);
    set_rotation(current_rot);
}

// ─── Click handler — single unified entry point through GameManager ────
void ChessPiece::on_click(Node *camera, const Ref<InputEvent> &event,
                          const Vector3 &position, const Vector3 &normal, int shape_idx) {
    if (is_captured) return;
    (void)camera; (void)position; (void)normal; (void)shape_idx; // fixed by Area3D.input_event's signal signature
    InputEventMouseButton *mb = Object::cast_to<InputEventMouseButton>(event.ptr());
    if (mb && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        GameManager *gm = GameManager::get_singleton();
        if (gm) {
            // Clicking a piece is equivalent to clicking the square it sits
            // on. GameManager decides: select / move-here / switch-selection.
            gm->handle_square_click(piece_col, piece_row);
        }
        get_viewport()->set_input_as_handled();
    }
}

// ─── Visual selection state (driven by GameManager) ────────────────────
void ChessPiece::set_selected_visual(bool selected) {
    if (selected == is_selected_visual) return;
    is_selected_visual = selected;

    if (is_selected_visual) {
        target_position.y = board_surface_y + LIFT_HEIGHT;
        show_highlights();
    } else {
        target_position.y = board_surface_y;
        hide_highlights();
    }
}

// ─── Colour property ──────────────────────────────────────────────────
void   ChessPiece::set_piece_color(const String &color) { piece_color = color; }
String ChessPiece::get_piece_color() const              { return piece_color;  }

// ─── Grid position management ─────────────────────────────────────────
void ChessPiece::set_grid_position(int col, int row) {
    piece_col = col;
    piece_row = row;
    target_position = Vector3(col_to_x(col), board_surface_y, row_to_z(row));
}

void ChessPiece::set_grid_position_silent(int col, int row) {
    piece_col = col;
    piece_row = row;
    target_position = Vector3(col_to_x(col), board_surface_y, row_to_z(row));
    set_position(target_position);
    target_rotation_y = original_rotation_y;
    Vector3 current_rot = get_rotation();
    current_rot.y = target_rotation_y;
    set_rotation(current_rot);
}

// ─── Identity helpers ──────────────────────────────────────────────────
char ChessPiece::get_type_char() const {
    switch (get_type()) {
        case PieceType::PAWN:   return 'P';
        case PieceType::KNIGHT: return 'N';
        case PieceType::BISHOP: return 'B';
        case PieceType::ROOK:   return 'R';
        case PieceType::QUEEN:  return 'Q';
        case PieceType::KING:   return 'K';
        default: return ' ';
    }
}

String ChessPiece::get_type_name() const {
    switch (get_type()) {
        case PieceType::PAWN:   return "pawn";
        case PieceType::KNIGHT: return "knight";
        case PieceType::BISHOP: return "bishop";
        case PieceType::ROOK:   return "rook";
        case PieceType::QUEEN:  return "queen";
        case PieceType::KING:   return "king";
        default: return "none";
    }
}

// ─── Base implementations (override in subclasses) ────────────────────
bool ChessPiece::is_valid_move(int /*from_col*/, int /*from_row*/,
                               int /*to_col*/,   int /*to_row*/) {
    return false;
}

std::vector<std::pair<int,int>> ChessPiece::get_valid_moves(int /*col*/, int /*row*/) {
    return {};
}

// ─── Highlight squares ────────────────────────────────────────────────
void ChessPiece::show_highlights() {
    hide_highlights();

    GameManager *gm = GameManager::get_singleton();
    if (!gm) return;

    // Advanced mode: no highlights shown
    if (!gm->get_highlights_enabled()) return;

    auto moves = gm->get_fully_legal_moves(this, piece_col, piece_row);

    for (auto &mv : moves) {
        int mc = mv.first, mr = mv.second;
        float cx = col_to_x(mc);
        float cz = row_to_z(mr);
        float half = SQUARE_SIZE * 0.5f;
        float border = SQUARE_SIZE * 0.06f;  // border thickness = 6% of square size

        bool is_capture = gm->is_occupied(mc, mr);

        // Light blue for normal moves, red-orange for captures
        Color border_color = is_capture
            ? Color(0.90f, 0.25f, 0.20f, 0.95f)
            : Color(0.35f, 0.75f, 1.00f, 0.95f);  // light blue

        // Draw 4 border strips: top, bottom, left, right
        // Each strip is a thin PlaneMesh positioned at the edge of the square.
        struct Strip { float ox; float oz; float w; float h; };
        Strip strips[4] = {
            {  0.0f,   -(half - border * 0.5f),  SQUARE_SIZE,         border },  // top (near Z)
            {  0.0f,    (half - border * 0.5f),  SQUARE_SIZE,         border },  // bottom (far Z)
            { -(half - border * 0.5f),  0.0f,    border,        SQUARE_SIZE  },  // left
            {  (half - border * 0.5f),  0.0f,    border,        SQUARE_SIZE  },  // right
        };

        for (int s = 0; s < 4; s++) {
            MeshInstance3D* hl = memnew(MeshInstance3D);

            Ref<PlaneMesh> mesh;
            mesh.instantiate();
            mesh->set_size(Vector2(strips[s].w, strips[s].h));

            Ref<StandardMaterial3D> mat;
            mat.instantiate();
            mat->set_albedo(border_color);
            mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
            mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);

            hl->set_mesh(mesh);
            hl->set_surface_override_material(0, mat);
            hl->set_as_top_level(true);
            add_child(hl);
            hl->set_global_position(Vector3(cx + strips[s].ox, 0.002f, cz + strips[s].oz));
            highlights.push_back(hl);
        }
    }
}

void ChessPiece::hide_highlights() {
    for (Node3D* h : highlights) {
        h->queue_free();
    }
    highlights.clear();
}

// ─── Capture / undo visual handling ─────────────────────────────────────
void ChessPiece::set_captured_visual(bool captured, int captured_slot_index) {
    is_captured = captured;

    if (captured) {
        hide_highlights();
        // Park on the silver border instead of hiding.
          float park_x = (piece_color == "white" ? 0.30f : -0.30f);
          float park_z = -0.22f + static_cast<float>(captured_slot_index - 1) * 0.032f;
        target_position = Vector3(park_x, board_surface_y, park_z);
        target_rotation_y = original_rotation_y - 1.570796f; // -PI / 2
    } else {
        // Restoration: caller (GameManager::undo) is responsible for
        // calling set_grid_position_silent() right after this to put the
        // piece back on its proper square; this just re-enables it.
        target_rotation_y = original_rotation_y;
    }
}
