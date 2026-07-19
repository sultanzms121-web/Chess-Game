#ifndef CHESS_PIECE_H
#define CHESS_PIECE_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <vector>
#include <utility>

namespace godot {

// Piece kind — used for promotion, notation, and the Memento snapshot.
enum class PieceType : int {
    NONE = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

class ChessPiece : public Node3D {
    GDCLASS(ChessPiece, Node3D)

private:
    bool is_selected_visual;
    Vector3 target_position;
    float target_rotation_y;
    float original_rotation_y;
    bool is_initialized;
    float board_surface_y;

    std::vector<Node3D*> highlights;

protected:
    static void _bind_methods();

    int piece_col;
    int piece_row;
    String piece_color; // "white" or "black"
    bool has_moved;
    bool is_captured;

public:
    ChessPiece();
    ~ChessPiece();

    void _ready() override;
    void _process(double delta) override;

    // Click entry point — forwards to GameManager::handle_square_click.
    void on_click(Node *camera, const Ref<InputEvent> &event, const Vector3 &position, const Vector3 &normal, int shape_idx);

    // ── Visual selection (driven BY GameManager, not self-authoritative) ──
    void set_selected_visual(bool selected);
    bool get_is_selected() const { return is_selected_visual; }

    // ── Colour property ──
    void   set_piece_color(const String &color);
    String get_piece_color() const;

    // ── Grid position ──
    int get_col() const { return piece_col; }
    int get_row() const { return piece_row; }
    void set_grid_position(int col, int row);          // animated move
    void set_grid_position_silent(int col, int row);    // instant snap (undo / init)

    bool get_has_moved() const { return has_moved; }
    void set_has_moved(bool v) { has_moved = v; }

    bool get_is_captured() const { return is_captured; }

    // ── Identity ──
    virtual PieceType get_type() const { return PieceType::NONE; }
    char get_type_char() const;
    String get_type_name() const;

    // ── Virtual movement rules — pure geometry, overridden per subclass ──
    virtual bool is_valid_move(int from_col, int from_row, int to_col, int to_row);
    virtual std::vector<std::pair<int,int>> get_valid_moves(int col, int row);

    // ── Highlights (called by GameManager when this piece becomes selected) ──
    void show_highlights();
    void hide_highlights();

    // ── Capture / undo visual handling ──────────────────────────────
    // A "captured" piece is hidden and parked off the playable area
    // rather than freed, so Undo can bring it straight back without
    // re-instantiating a GDExtension object mid-game.
    void set_captured_visual(bool captured, int captured_slot_index = 0);
};

}

#endif // CHESS_PIECE_H
