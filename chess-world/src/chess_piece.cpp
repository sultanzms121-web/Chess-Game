#include "chess_piece.h"
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

// --- UPDATED TO MATCH YOUR GODOT COLLISION SIZE ---
const float PLAYABLE_AREA = 0.62f; 
const float SQUARE_SIZE = PLAYABLE_AREA / 8.0f; // Automatically calculates the new larger squares
const float BOARD_HALF = PLAYABLE_AREA / 2.0f;  
const float CELL_HALF = SQUARE_SIZE / 2.0f; 
const float LIFT_HEIGHT = 0.05f; 

void ChessPiece::_bind_methods() {
    ClassDB::bind_method(D_METHOD("toggle_select"), &ChessPiece::toggle_select);
    ClassDB::bind_method(D_METHOD("on_click", "camera", "event", "position", "normal", "shape_idx"), &ChessPiece::on_click);
    ClassDB::bind_method(D_METHOD("move_to_if_selected", "new_pos"), &ChessPiece::move_to_if_selected);
}

ChessPiece::ChessPiece() {
    is_selected = false;
    is_initialized = false;
    board_surface_y = 0.0f;
    set_process(true);
}

ChessPiece::~ChessPiece() {}

void ChessPiece::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) return;

    if (!is_initialized) {
        Vector3 start_pos = get_position(); 
        
        // UNIFIED AUTO-SNAP: Uses the exact same grid logic as the mouse clicks
        int col = std::floor((start_pos.x + BOARD_HALF) / SQUARE_SIZE);
        int row = std::floor((start_pos.z + BOARD_HALF) / SQUARE_SIZE);
        
        if (col < 0) col = 0; if (col > 7) col = 7;
        if (row < 0) row = 0; if (row > 7) row = 7;
        
        float snap_x = -BOARD_HALF + (col * SQUARE_SIZE) + CELL_HALF;
        float snap_z = -BOARD_HALF + (row * SQUARE_SIZE) + CELL_HALF;
        
        target_position  = Vector3(snap_x, start_pos.y, snap_z);
        board_surface_y  = start_pos.y;
        is_initialized   = true;
    }

    Vector3 current_pos = get_position();
    set_position(current_pos.lerp(target_position, 15.0 * delta));
}

void ChessPiece::toggle_select() {
    is_selected = !is_selected;
    if (is_selected) {
        target_position.y = board_surface_y + LIFT_HEIGHT;
        show_highlights();
    } else {
        target_position.y = board_surface_y; 
        hide_highlights();
    }
}

void ChessPiece::on_click(Node *camera, const Ref<InputEvent> &event, const Vector3 &position, const Vector3 &normal, int shape_idx) {
    InputEventMouseButton *mouse_event = Object::cast_to<InputEventMouseButton>(event.ptr());
    if (mouse_event && mouse_event->is_pressed() && mouse_event->get_button_index() == MOUSE_BUTTON_LEFT) {
        toggle_select(); 
        get_viewport()->set_input_as_handled(); 
    }
}

// ---------------------------------------------------------
// STRICT AREA BOUNDARY KING ALGORITHM
// ---------------------------------------------------------
void ChessPiece::move_to_if_selected(Vector3 new_pos) {
    if (!is_selected) return;

    // 1. Calculate the strict Area Index (0 to 7) by dividing from the absolute bottom-left edge (-BOARD_HALF)
    int clicked_col = std::floor((new_pos.x + BOARD_HALF) / SQUARE_SIZE);
    int clicked_row = std::floor((new_pos.z + BOARD_HALF) / SQUARE_SIZE);

    // 2. Reject clicks outside the 8x8 playable area
    if (clicked_col < 0 || clicked_col > 7 || clicked_row < 0 || clicked_row > 7) return;

    // 3. Calculate the King's current Area Index based on his exact position
    int current_col = std::floor((target_position.x + BOARD_HALF) / SQUARE_SIZE);
    int current_row = std::floor((target_position.z + BOARD_HALF) / SQUARE_SIZE);

    // 4. Calculate grid distance
    int delta_col = std::abs(clicked_col - current_col);
    int delta_row = std::abs(clicked_row - current_row);

    // 5. KING RULE: Only allow if the clicked area is exactly 1 square away
    if ((delta_col <= 1 && delta_row <= 1) && !(delta_col == 0 && delta_row == 0)) {
        
        // 6. Calculate the dead-center point of that specific area to place the King
        float final_x = -BOARD_HALF + (clicked_col * SQUARE_SIZE) + CELL_HALF;
        float final_z = -BOARD_HALF + (clicked_row * SQUARE_SIZE) + CELL_HALF;
        
        target_position = Vector3(final_x, board_surface_y, final_z);
        toggle_select();
    }
}

// ---------------------------------------------------------
// UNIFIED HIGHLIGHTS: Grid Based Neighbor Checking
// ---------------------------------------------------------
void ChessPiece::show_highlights() {
    hide_highlights();

    int current_col = std::floor((target_position.x + BOARD_HALF) / SQUARE_SIZE);
    int current_row = std::floor((target_position.z + BOARD_HALF) / SQUARE_SIZE);

    // Loop through the 8 surrounding grid slots (-1, 0, 1)
    for (int step_x = -1; step_x <= 1; step_x++) {
        for (int step_z = -1; step_z <= 1; step_z++) {
            
            // Skip the center square
            if (step_x == 0 && step_z == 0) continue; 

            int neighbor_col = current_col + step_x;
            int neighbor_row = current_row + step_z;

            // Only spawn a highlight if the neighbor square actually exists on the board
            if (neighbor_col >= 0 && neighbor_col <= 7 && neighbor_row >= 0 && neighbor_row <= 7) {
                
                float hx = -BOARD_HALF + (neighbor_col * SQUARE_SIZE) + CELL_HALF;
                float hz = -BOARD_HALF + (neighbor_row * SQUARE_SIZE) + CELL_HALF;

                MeshInstance3D* highlight = memnew(MeshInstance3D);
                
                Ref<PlaneMesh> mesh;
                mesh.instantiate();
                mesh->set_size(Vector2(SQUARE_SIZE * 0.85f, SQUARE_SIZE * 0.85f));

                Ref<StandardMaterial3D> mat;
                mat.instantiate();
                mat->set_albedo(Color(1.0f, 0.85f, 0.2f, 0.55f));
                mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
                mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);

                highlight->set_mesh(mesh);
                highlight->set_surface_override_material(0, mat);
                highlight->set_as_top_level(true);

                add_child(highlight);
                highlight->set_global_position(Vector3(hx, 0.001f, hz));
                highlights.push_back(highlight);
            }
        }
    }
}

void ChessPiece::hide_highlights() {
    for (Node3D* h : highlights) {
        h->queue_free(); 
    }
    highlights.clear();
}